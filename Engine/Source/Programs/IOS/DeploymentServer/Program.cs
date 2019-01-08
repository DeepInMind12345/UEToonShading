/**
 * Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.IO;
using Microsoft.Win32;
using iPhonePackager;
using Manzana;
using System.Net.Sockets;
using System.Net;
using System.Threading;
using System.Configuration;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Linq;

namespace DeploymentServer
{
	class Program
	{
		static int ExitCode = 0;
		const int DefaultPort = 41000;
		static int Port = DefaultPort;
		static bool IsRunningCommand = false;
		static bool IsStopping = false;
		static int ClientCounter = 0;
		static long TimeOut = 120000;
		static Stopwatch GlobalTimer = Stopwatch.StartNew();

		static string TestStartPath = null;

		class TcpClientInfo
		{
			public bool HasCommand
			{
				get
				{
					return bHasCommand;
				}
				set
				{
					bHasCommand = value;
				}
			}
			public bool KeepAlive
			{
				get
				{
					return bKeepAlive;
				}
			}
			public bool IsStillRunning
			{
				get { return !bCommandComplete; }
			}

			public bool NeedsResponse
			{
				get { return bNeedsResponse; }
			}

			public string LastCommand
			{
				get { return Command; }
			}

			public bool GetLastResult
			{
				get { return LastResult; }
			}

			protected bool bHasCommand = false;
			protected bool bNeedsResponse = false;
			protected string Command = "";
			protected List<string> FileList = new List<string>();
			protected string Bundle = "";
			protected string Manifest = "";
			protected string IpaPath = "";
			protected string Device = "";
			protected string Param = "";
			protected bool bKeepAlive = false;
			protected bool LastResult = false;

			System.Threading.Thread runLoop = null;
			bool bCommandComplete = true;

			public void StopRun()
			{
				try
				{
					if (runLoop != null)
					{
						runLoop.Abort();
					}
				}
				catch { }
			}

			public bool ParseCommand(List<string> Arguments)
			{
				if (Arguments.Count >= 0)
				{
					
					int Start = 0;
					while(Arguments[Start].StartsWith("-"))
					{
						Start++;
						if (Start >= Arguments.Count)
						{
							return false;
						}
					}
					bKeepAlive = true;
					Command = Arguments[Start].ToLowerInvariant();
					FileList.Clear();
					for (int ArgIndex = 0; ArgIndex < Arguments.Count; ArgIndex++)
					{
						Arguments[ArgIndex] = Arguments[ArgIndex].Trim('"');
					}
					for (int ArgIndex = Start + 1; ArgIndex < Arguments.Count; ArgIndex++)
					{
						string Arg = Arguments[ArgIndex].ToLowerInvariant();
						if (Arg.StartsWith("-"))
						{
							switch (Arg)
							{
								case "-file":
									if (Arguments.Count > ArgIndex + 1)
									{
										FileList.Add(Arguments[++ArgIndex]);
									}
									else
									{
										return false;
									}
									break;

								case "-bundle":
									if (Arguments.Count > ArgIndex + 1)
									{
										Bundle = Arguments[++ArgIndex];
									}
									else
									{
										return false;
									}
									break;

								case "-manifest":
									if (Arguments.Count > ArgIndex + 1)
									{
										Manifest = Arguments[++ArgIndex];
									}
									else
									{
										return false;
									}
									break;

								case "-ipa":
									if (Arguments.Count > ArgIndex + 1)
									{
										IpaPath = Arguments[++ArgIndex];
									}
									else
									{
										return false;
									}
									break;

								case "-device":
									if (Arguments.Count > ArgIndex + 1)
									{
										Device = Arguments[++ArgIndex];
									}
									else
									{
										return false;
									}
									break;
								case "-param":
									if (Arguments.Count > ArgIndex + 1)
									{
										Param = Arguments[++ArgIndex];
									}
									else
									{
										return false;
									}
									break;
								case "-nokeepalive":
									bKeepAlive = false;
									break;
								case "-timeout":
									{
										if (Arguments.Count > ArgIndex + 1)
										{
											long ArgTime = TimeOut;
											long.TryParse(Arguments[++ArgIndex], out ArgTime);
											if (ArgTime > 0)
											{
												TimeOut = ArgTime;
												Console.WriteLine(string.Format("Deployment Server timeout set to {0} (remote)", TimeOut.ToString()));
											}
										}
										break;
									}
							}
						}
					}
				}
				return true;
			}

			public int RunCommand(TextWriter Writer)
			{
				TextWriter ConsoleOld = Console.Out;

				LastResult = true;
				bCommandComplete = false;
				bool bWaitForCompletion = ShouldWaitForCompletion(Command);
				bNeedsResponse = !bWaitForCompletion;

				runLoop = new System.Threading.Thread(delegate ()
				{
					try
					{
						DeploymentProxy.Deployer.DeviceId = Device;

						switch (Command)
						{
							case "stop":
								Console.SetOut(Writer);
								Console.WriteLine("Deployment Server Stopping ...");
								IsStopping = true;
								int StopTimeout = 12;
								while (ClientCounter > 1) // wait for other threads to stop so the client requesting to stop to block until safely stopped
								{
									CoreFoundationRunLoop.RunLoopRunInMode(CoreFoundationRunLoop.kCFRunLoopDefaultMode(), 1.0, 0);
									System.Threading.Thread.Sleep(50);
									StopTimeout--;
									if (StopTimeout <= 0)
									{
										ForceKillProcesses();
										break;
									}
								}
								break;
							case "backup":
								Console.SetOut(Writer);
								LastResult = DeploymentProxy.Deployer.BackupFiles(Bundle, FileList.ToArray());
								break;

							case "deploy":
								Console.SetOut(Writer);
								LastResult = DeploymentProxy.Deployer.InstallFilesOnDevice(Bundle, Manifest);
								break;

							case "copyfile":
								Console.SetOut(Writer);
								LastResult = DeploymentProxy.Deployer.CopyFileToDevice(Bundle, FileList[0], FileList[1]);
								break;

							case "install":
								Console.SetOut(Writer);
								LastResult = DeploymentProxy.Deployer.InstallIPAOnDevice(IpaPath);
								break;

							case "enumerate":
								Console.SetOut(Writer);
								DeploymentProxy.Deployer.EnumerateConnectedDevices();
								break;

							case "listdevices":
								Console.SetOut(Writer);
								DeploymentProxy.Deployer.ListDevices();
								break;

							case "command":
								if (Device.Length < 5)
								{
									Console.WriteLine("Device ID not present.");
								}
								else if (Param.Length < 1)
								{
									Console.WriteLine("Parameter not present.");
								}
								else
								{
									try
									{
										DeploymentProxy.Deployer.TunnelToDevice(Device, Param);
									}
									catch
									{
										Console.WriteLine("Errors encountered while tunneling to device.");
									}
								}
								break;

							case "listentodevice":
								if (Device.Length < 5)
								{
									Console.WriteLine("Device ID not present.");
								}
								else
								{
									DeploymentProxy.Deployer.ListenToDevice(Device, Writer);
								}
								break;
						}
					}
					catch (Exception e)
					{
						Console.SetOut(ConsoleOld);
						if (Command != "stop")
						{
							Console.WriteLine("Exception: {0}", e);
						}
						LastResult = false;
					}
					finally
					{
						Writer.Flush();
						Console.SetOut(ConsoleOld);
						bCommandComplete = true;
					}
					
				});
				try
				{
					runLoop.Start();
					if (bWaitForCompletion)
					{
						while (!bCommandComplete)
						{
							CoreFoundationRunLoop.RunLoopRunInMode(CoreFoundationRunLoop.kCFRunLoopDefaultMode(), 1.0, 0);
						}
					}
				}
				catch (Exception e)
				{
					
					if (Command != "stop")
					{
						Console.WriteLine("Exception: {0}", e);
					}
					bCommandComplete = false;
					LastResult = false;
				}
				finally
				{
						
				}

				return LastResult ? 0 : 1;
			}

			public static bool NeedsVersionCheck(String Command)
			{
				switch (Command)
				{
					case "stop":
					case "listdevices":
					case "listentodevice":
						return false;
				}
				return true;
			}
			public static bool ShouldWaitForCompletion(String Command)
			{
				switch (Command)
				{
					case "listentodevice":
						return false;
				}
				return true;
			}
		}

		static String GetDeploymentServerPath()
		{
			return Path.GetDirectoryName(AppDomain.CurrentDomain.BaseDirectory);
		}

		static void CreateDeploymentServerProcess()
		{
			Process NewProcess = new Process();

			NewProcess.StartInfo.WorkingDirectory = GetDeploymentServerPath();

			if (Environment.OSVersion.Platform == PlatformID.MacOSX || Environment.OSVersion.Platform == PlatformID.Unix)
			{
				NewProcess.StartInfo.WorkingDirectory.TrimEnd('/');
				NewProcess.StartInfo.FileName = NewProcess.StartInfo.WorkingDirectory + "/../../../Build/BatchFiles/Mac/RunMono.sh";
				NewProcess.StartInfo.Arguments = "\"" + NewProcess.StartInfo.WorkingDirectory + "/DeploymentServer.exe\" -iphonepackager " + Process.GetCurrentProcess().Id.ToString() + " " + NewProcess.StartInfo.WorkingDirectory;
			}
			else
			{
				NewProcess.StartInfo.WorkingDirectory.TrimEnd('\\');
				NewProcess.StartInfo.FileName = NewProcess.StartInfo.WorkingDirectory + "\\DeploymentServerLauncher.exe";
			}
			NewProcess.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
			NewProcess.StartInfo.UseShellExecute = true;

			try
			{
				NewProcess.Start();
				System.Threading.Thread.Sleep(500);
			}
			catch (System.Exception ex)
			{
				Console.WriteLine("Failed to create deployment server process ({0})", ex.Message);
			}
		}

		static TcpClient IsServiceRegistered()
		{
			TcpClient Client = null;
			try
			{
				Client = new TcpClient("localhost", Port);
			}
			catch
			{
				if (Client != null)
				{
					Client.Close();
				}
				Client = null;
			}

			return Client;
		}

		static void CreateDeploymentInterface()
		{
			DeploymentProxy.Deployer = new DeploymentImplementation();

			IpcServerChannel Channel = new IpcServerChannel("iPhonePackager");
			ChannelServices.RegisterChannel(Channel, false);
			RemotingConfiguration.RegisterWellKnownServiceType(typeof(DeploymentProxy), "DeploymentServer_PID", WellKnownObjectMode.Singleton);
		}

		protected static void ParseServerParam(string[] Arguments)
		{
			if (Arguments.Length > 2)
			{
				TestStartPath = Arguments[2];
			}
			else
			{
				TestStartPath = GetDeploymentServerPath();
			}
			if (Arguments.Length > 3)
			{
				for (int ArgIndex = 3; ArgIndex < Arguments.Length; ArgIndex++)
				{
					string Arg = Arguments[ArgIndex].ToLowerInvariant();
					if (Arg.StartsWith("-"))
					{
						switch (Arg)
						{
							case "-timeout":
								{
									if (Arguments.Length > ArgIndex + 1)
									{
										long ArgTime = TimeOut;
										long.TryParse(Arguments[++ArgIndex], out ArgTime);
										if (ArgTime > 0)
										{
											TimeOut = ArgTime;
											Console.WriteLine(string.Format("Deployment Server timeout set to {0}", TimeOut.ToString()));
										}
									}
									break;
								}
						}
					}
				}
			}
		}

		/**
		 *	Main Server Loop
		 */
		static void ServerLoop(TcpClient IsServiceRunning, string[] Args)
		{
			Program.ExitCode = 0;

			if (IsServiceRunning != null)
				return;

			bool bCreatedMutex = false;
			String MutexName = "Global\\DeploymentServer_Mutex_SERVERINSTANCE";
			Mutex DSBlockMutex = null;

			try
			{
				DSBlockMutex = new Mutex(true, MutexName, out bCreatedMutex);
				if (!bCreatedMutex)
				{
					// running is not allowed (perhaps a another server instance is running, but on a different port or is not responding to a connection request)
					return;
				}
			}
			catch
			{
				return;
			}

			TcpListener Server = null;
			FileStream OutSm = null;
			TextWriter Writer = null;
			TextWriter OldConsole = Console.Out;
			System.Threading.Thread ProcessClient = null;
			try
			{
				CreateDeploymentInterface();
				long.TryParse(ConfigurationManager.AppSettings["DSTimeOut"], out TimeOut);
				if (TimeOut < 30000)
				{
					TimeOut = 30000;
				}
				Server = new TcpListener(IPAddress.Any, Port);
				Server.Start();
				OutSm = new FileStream("DeploymentServer.log", FileMode.Create, FileAccess.Write);
				Writer = new StreamWriter(OutSm);
				Console.SetOut(Writer);

				ParseServerParam(Args);
				Console.WriteLine(string.Format("Deployment Server listening to port {0}", Port.ToString()));
				Console.WriteLine(string.Format("Deployment Server inactivity timeout {0}", TimeOut.ToString()));
				Console.WriteLine("---------------------------------------------------------");

				// Processing commands

				ProcessClient = new System.Threading.Thread(delegate ()
				{
					try
					{
						int localClientID = 0;
						while (true)
						{
							TcpClient client = Server.AcceptTcpClient();
							
							TrackTCPClient(client, localClientID);
							localClientID++;
							System.Threading.Thread.Sleep(100);
						}
					}
					catch
					{

					}
				});
				ProcessClient.Start();

				// this will exit on its own after the set inactivity time or by a kill command or by a remote "stop" command
				while (true)
				{
					CoreFoundationRunLoop.RunLoopRunInMode(CoreFoundationRunLoop.kCFRunLoopDefaultMode(), 1.0, 0);
					System.Threading.Thread.Sleep(50);
					Writer.Flush();
					OutSm.Flush();
					if (ClientCounter <= 0)
					{
						if (IsStopping)
						{
							Console.WriteLine("Deployment Server IsStopping exit.");
							break;
						}
						if (GlobalTimer.ElapsedMilliseconds > TimeOut)
						{
							Console.WriteLine("Deployment Server inactivity timeout.");
							IsStopping = true;
							break;
						}
					}
				}
			}
			catch (SocketException e)
			{
				Console.WriteLine("SocketException: {0}", e);
			}
			catch (System.Exception Ex)
			{
				Console.WriteLine("Exception: {0}", Ex);
				Console.WriteLine("Stack: {0}", Ex.StackTrace);
				Console.WriteLine("Inner: {0}", Ex.InnerException.Message);
			}
			finally
			{
				if (DSBlockMutex != null)
				{
					DSBlockMutex.ReleaseMutex();
					DSBlockMutex.Dispose();
					DSBlockMutex = null;
				}
				if (ProcessClient != null)
				{
					ProcessClient.Abort();
				}
				if (Server != null)
				{
					Server.Stop();
				}
				if (Writer != null)
				{
					Writer.Close();
				}
				if (OutSm != null)
				{
					OutSm.Close();
				}
			}
			Console.SetOut(OldConsole);
			Console.WriteLine("Deployment Server Stopped.");
		}

		static void RunLocalInstance(string[] Args)
		{
			// running as one instance only
			CreateDeploymentInterface();
			TcpClientInfo LocalClientInfo = new TcpClientInfo();
			List<string> Arguments = new List<string>();
			Arguments.AddRange(Args);

			if (LocalClientInfo.ParseCommand(Arguments))
			{
				LocalClientInfo.HasCommand = true;
				Console.WriteLine("Running as local instance");
				LocalClientInfo.RunCommand(Console.Out);
				while (LocalClientInfo.IsStillRunning)
				{
					Thread.Sleep(100);
				}
				bool RetCode = LocalClientInfo.GetLastResult;
			}
			Console.WriteLine("Deployment Server ended local instance.");
		}

		/**
		 * Main Client loop
		 */
		static void ClientLoop(TcpClient IsServiceRunning, string[] Args, string LocalCommand)
		{
			if (IsServiceRunning == null)
			{
				if (Args.Length < 1 || LocalCommand == "stop")
				{
					Console.WriteLine("Deployment Server not running ...");
					ForceKillProcesses();
					return;
				}
				// on mac we only start one local instance due to mono limitations
				if (Environment.OSVersion.Platform == PlatformID.MacOSX || Environment.OSVersion.Platform == PlatformID.Unix || Args[0].Equals("-standalone"))
				{
					RunLocalInstance(Args);
					return;
				}
				else
				{
					CreateDeploymentServerProcess();
				}
			}
			// Parse the command
			TcpClient Client = IsServiceRunning;
			try
			{
				int RetryCount = 3;
				while (RetryCount > 0 && Client == null)
				{
					try
					{
						if (Client == null)
						{
							Client = new TcpClient("localhost", Port); //The client gets here
						}
					}
					catch (Exception e)
					{
						RetryCount--;
						Client = null;
						if (RetryCount <= 0)
						{
							throw (e);
						}
						System.Threading.Thread.Sleep(500);
					}
				}
				StreamReader clientIn = new StreamReader(Client.GetStream());
				StreamWriter clientOut = new StreamWriter(Client.GetStream());
				{
					string Response = clientIn.ReadLine();
					if (TcpClientInfo.NeedsVersionCheck(LocalCommand))
					{

						if (!Response.Equals("DIR" + GetDeploymentServerPath(), StringComparison.InvariantCultureIgnoreCase))
						{
							Console.WriteLine("Wrong server running, restarting the server ...");
							clientOut.Write("stop");
							clientOut.Write("\r");
							clientOut.Flush();
							while (true)
							{
								Response = clientIn.ReadLine();
								if (Response.EndsWith("CMDOK") || Response.EndsWith("CMDFAIL"))
								{
									break;
								}
								else
								{
									Console.WriteLine(Response);
								}
								ForceKillProcesses();
							}
							if (Client != null)
							{
								Client.Close();
							}
							Client = null;
							CreateDeploymentServerProcess();
							IsServiceRunning = IsServiceRegistered();
							Client = IsServiceRunning;
							clientIn = new StreamReader(Client.GetStream());
							clientOut = new StreamWriter(Client.GetStream());
						}
					}
				}

				for (int ArgIndex = 0; ArgIndex < Args.Length; ArgIndex++)
				{
					if (Args[ArgIndex].Contains(' '))
					{
						clientOut.Write('"' + Args[ArgIndex] + '"' + ' ');
					}
					else
					{
						clientOut.Write(Args[ArgIndex] + ' ');
					}
				}
				clientOut.Write("\r");
				clientOut.Flush();

				while (true)
				{
					string Response = clientIn.ReadLine();
					if (Response.EndsWith("CMDOK"))
					{
						Program.ExitCode = 0;
						break;
					}
					else if (Response.EndsWith("CMDFAIL"))
					{
						Program.ExitCode = 1;
						break;
					}
					else
					{
						Console.WriteLine(Response);
					}
				}
			}
			catch (Exception e)
			{
				if (LocalCommand != "stop")
				{
					Console.WriteLine("Exception: {0}", e);
				}
			}
			finally
			{
				if (Client != null)
				{
					Client.Close();
				}
			}
		}

		static int Main(string[] Args)
		{
			string LocalCommand = "";
			if (Args.Length > 0)
			{
				LocalCommand = Args[0].ToLowerInvariant();
			}
			else
			{
				Console.WriteLine("Deployment Server usage: ");
				Console.WriteLine("DeploymentServer.exe <command> [<parameter> [<value>] ...]");
				Console.WriteLine("Valid Commands:");
				Console.WriteLine("\t stop");
				Console.WriteLine("\t backup");
				Console.WriteLine("\t deploy");
				Console.WriteLine("\t copyfile");
				Console.WriteLine("\t install");
				Console.WriteLine("\t enumerate");
				Console.WriteLine("\t listdevices");
				Console.WriteLine("\t listentodevice");
				Console.WriteLine("\t -iphonepackager");
				Console.WriteLine("Valid Parameters:");
				Console.WriteLine("\t -file <filename>");
				Console.WriteLine("\t -bundle <bundle name>");
				Console.WriteLine("\t -manifest <manifest file>");
				Console.WriteLine("\t -ipa <ipa path>");
				Console.WriteLine("\t -device <device ID>");
				Console.WriteLine("\t -nokeepalive");
				Console.WriteLine("\t -timeout <miliseconds>");
				Console.WriteLine("");

				return 0;
			}
			try
			{
				if (LocalCommand != "stop")
				{
					bool bCreatedMutex = false;
					String MutexName = "Global\\DeploymentServer_Mutex_RestartNotAllowed";
					Mutex DSBlockMutex = new Mutex(true, MutexName, out bCreatedMutex);
					if (!bCreatedMutex)
					{
						// running is not allowed (perhaps a build command is in progress)
						Program.ExitCode = 1;
						return 1;
					}
					DSBlockMutex.ReleaseMutex();
					DSBlockMutex.Dispose();
					DSBlockMutex = null;
				}
			}
			catch
			{
				Program.ExitCode = 0;
				return 0;
			}
			int.TryParse(ConfigurationManager.AppSettings["DSPort"], out Port);
			if (Port < 1 || Port > 65535)
			{
				Port = DefaultPort;
			}

			TcpClient IsServiceRunning = IsServiceRegistered();

			// parrent ID not needed anymore
			if (Args[0].Equals("-iphonepackager"))
			{
				ServerLoop(IsServiceRunning, Args);
			}
			else
			{
				ClientLoop(IsServiceRunning, Args, LocalCommand);
			}

			Environment.ExitCode = Program.ExitCode;
			return Program.ExitCode;
		}
		
		private static string ProcessTCPRequest(string ClientData, ref TcpClientInfo Client)
		{
			string[] Commands = ClientData.Split('\n');
			string Ret = "";
			if (Commands.Length >= 1)
			{
				// hm ... this will always process only the last command
				for (int i = 0; i < Commands.Length; i++)
				{
					if (i < Commands.Length - 1 || Commands[i].EndsWith("\r"))
					{
						List<string> Arguments = Regex.Matches(Commands[i], @"[\""].+?[\""]|[^ ]+")
												.Cast<Match>()
												.Select(m => m.Value)
												.ToList();
						if (Client.ParseCommand(Arguments))
						{
							Client.HasCommand = true;
						}
					}
					else
					{
						Ret = Commands[i];
					}
				}
			}
			return Ret;
		}

		private static void TrackTCPClient(TcpClient Client, int localID)
		{
			string LastCommand = "";
			System.Threading.Thread processTracker = new System.Threading.Thread(delegate ()
			{
				ClientCounter++;
				TcpClientInfo ClientInfo = null;
				try
				{
					NetworkStream ClStream = Client.GetStream();
					string ClientIP = ((IPEndPoint)Client.Client.RemoteEndPoint).Address.ToString();
					Console.WriteLine("Client [{0}] IP:{1} connected.", localID, ClientIP);

					TextWriter Writer = new StreamWriter(ClStream);
					
					Writer.WriteLine("DIR" + TestStartPath);
					Writer.Flush();

					Byte[] Buffer = new Byte[2048];
					string Unprocessed = "";
					ClientInfo = new TcpClientInfo();
					
					while (true)
					{
						//Console.WriteLine("Looping [{0}]", localID);
						if (ClientInfo.HasCommand && !IsStopping)
						{
							Console.WriteLine("Got command [{0}]", localID);
							if (!IsRunningCommand && !ClientInfo.IsStillRunning)
							{
								ClientInfo.HasCommand = false;
								IsRunningCommand = true;

								try
								{
									LastCommand = ClientInfo.LastCommand;
									ClientInfo.RunCommand(Writer);
								}
								catch
								{

								}
								IsRunningCommand = false;
								if (!ClientInfo.IsStillRunning)
								{
									Writer.WriteLine(ClientInfo.GetLastResult ? "\nCMDOK" : "\nCMDFAIL");
									Writer.Flush();
									ClStream.Flush();
									break;
								}
							}
							else if (ClientInfo.LastCommand == "stop")
							{
								ClientInfo.RunCommand(Writer);
							}
						}
						
						if (ClStream.DataAvailable)
						{
							int Bytes = ClStream.Read(Buffer, 0, Buffer.Length);
							string Cmd = Unprocessed + System.Text.Encoding.ASCII.GetString(Buffer, 0, Bytes);
							Unprocessed = ProcessTCPRequest(Cmd, ref ClientInfo);
							if (ClientInfo.KeepAlive)
							{
								GlobalTimer.Restart();
							}
						}
						else
						{
							System.Threading.Thread.Sleep(100);
						}
						if (ClientInfo.IsStillRunning)
						{
							bool BlockingState = Client.Client.Blocking;
							try
							{
								byte[] tmp = new byte[1];

								Client.Client.Blocking = false;
								Client.Client.Send(tmp, 0, 0);
							}
							catch (SocketException e)
							{
								// 10035 == WSAEWOULDBLOCK
								if (e.NativeErrorCode.Equals(10035))
								{
									//Console.WriteLine("Still Connected, but the Send would block");
								}
								else
								{
									// disconnected
									break;
								}
							}
							finally
							{
								Client.Client.Blocking = BlockingState;
							}
						}
						else if (ClientInfo.NeedsResponse)
						{
							Writer.WriteLine("\nCMDOK");
							Writer.Flush();
							ClStream.Flush();
							break;
						}
						if (IsStopping)
						{
							try
							{
								if (ClientInfo.HasCommand)
								{
									Writer.WriteLine("\nCMDFAIL");
								}
								else
								{
									Writer.WriteLine("\nCMDOK");

								}
								Writer.Flush();
								ClStream.Flush();
							}
							catch
							{
								// we'll go silent since we're closing
							}
							break;
						}
					}
				}
				catch (Exception e)
				{
					Console.WriteLine("Exception: {0}", e);
				}
				finally
				{
					if (ClientInfo != null)
					{
						ClientInfo.StopRun();
					}
					if (Client != null)
					{
						if (Client.Client != null && Client.Client.RemoteEndPoint != null)
						{
							Console.WriteLine("Client [{0}] disconnected ({1}).", localID, LastCommand);
						}
						Client.Close();
					}
				}
				ClientCounter--;
			});
			processTracker.Start();
		}

		static void ForceKillProcesses()
		{
			foreach (var process in Process.GetProcessesByName("DeploymentServer"))
			{
				if (process.Id != Process.GetCurrentProcess().Id)
				{
					process.Kill();
				}
			}
		}
	}
}
