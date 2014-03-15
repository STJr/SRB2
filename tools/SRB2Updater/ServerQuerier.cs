using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace SRB2Updater
{
    class ServerQuerier
    {
        private UdpClient udpclient;
        private IPEndPoint ipepMS;

        private const int MS_HOLEPUNCH_SIZE = 0;
        private const int PT_ASKINFO_SIZE = 16;
        private const byte PT_ASKINFO = 12;
        private const byte PT_SERVERINFO = 13;
        private const int MAXSERVERNAME = 32;
        private const int MAX_WADPATH = 128;

        /// <summary>
        /// Constructs a ServerQuerier object.
        /// </summary>
        public ServerQuerier()
        {
            udpclient = new UdpClient(0, AddressFamily.InterNetwork);

            // Fix for WSAECONNRESET. Only affects Win2k and up. If I send a
            // packet to a host which replies with an ICMP Port Unreachable,
            // subsequent socket operations go doo-lally. So, we enable the
            // older behaviour of ignoring these ICMP messages, since we don't
            // care about them anyway.
            if (Environment.OSVersion.Platform == PlatformID.Win32NT &&
                Environment.OSVersion.Version.Major >= 5)
            {
                const uint IOC_IN = 0x80000000;
                const uint IOC_VENDOR = 0x18000000;
                const uint SIO_UDP_CONNRESET = IOC_IN | IOC_VENDOR | 12;

                udpclient.Client.IOControl(unchecked((int)SIO_UDP_CONNRESET), new byte[] { Convert.ToByte(false) }, null);
            }
        }

        public void StartListening(ServerInfoReceiveHandler sirh)
        {
            // Start listening.
            udpclient.BeginReceive(new AsyncCallback(ServerInfoReceiveHandler.Receive), sirh);
        }


        /// <summary>
        /// Sets the master server address. Necessary before querying via the MS.
        /// </summary>
        /// <param name="strAddress">IP address of hostname of MS.</param>
        /// <param name="unPort">Port of MS.</param>
        public void SetMasterServer(string strAddress, ushort unPort)
        {
            IPAddress address = Dns.GetHostEntry(strAddress).AddressList[0];
            ipepMS = new IPEndPoint(address, unPort);
        }

        public void Query(string strAddress, ushort unPort)
        {
            // Build the packet.
            byte[] byPacket = new byte[PT_ASKINFO_SIZE];
            BinaryWriter bw = new BinaryWriter(new MemoryStream(byPacket));

            bw.Seek(4, SeekOrigin.Begin);		// Skip the checksum.
            bw.Write((byte)0);					// ack
            bw.Write((byte)0);					// ackreturn
            bw.Write((byte)PT_ASKINFO);			// Packet type.
            bw.Write((byte)0);					// Reserved.
            bw.Write((byte)0);					// Version. This is actually unnecessary -- the client will reply anyway. -MattW_CFI
            bw.Write((byte)0);					// Reserved.
            bw.Write((byte)0);					// Reserved.
            bw.Write((byte)0);					// Reserved.
            // Time for ping calculation.
            bw.Write(unchecked((uint)(DateTime.Now.Ticks / 10000)));

            // Calculate the checksum.
            bw.Seek(0, SeekOrigin.Begin);
            bw.Write(SRB2Checksum(byPacket));

            // Send the packet.
            udpclient.Send(byPacket, byPacket.Length, strAddress, unPort);
        }

        /// <summary>
        /// Calculates the checksum of an SRB2 packet.
        /// </summary>
        /// <param name="byPacket">Packet.</param>
        /// <returns>Checksum.</returns>
        private static uint SRB2Checksum(byte[] byPacket)
        {
            uint c = 0x1234567;
            int i;

            for (i = 4; i < byPacket.Length; i++)
                unchecked
                {
                    c += (uint)byPacket[i] * (uint)(i - 3);
                }

            return c;
        }

        private static string ReadFixedLengthStr(BinaryReader br, int iLen)
        {
            String str = Encoding.ASCII.GetString(br.ReadBytes(iLen));
            int iPos = str.IndexOf("\0");
            if (iPos >= 0)
                str = str.Remove(iPos);

            return str;
        }

        public abstract class ServerInfoReceiveHandler
        {
            UdpClient udpclient;
            IPEndPoint ipepRemote;

            /// <summary>
            /// Called after a server info packet is received.
            /// </summary>
            /// <param name="srb2si">Server info.</param>
            public abstract void ProcessServerInfo(SRB2ServerInfo srb2si);
            public abstract void HandleException(Exception e);

            public ServerInfoReceiveHandler(ServerQuerier sq)
            {
                ipepRemote = new IPEndPoint(IPAddress.Any, 0);

                udpclient = sq.udpclient;
            }

            public static void Receive(IAsyncResult ar)
            {
                ServerInfoReceiveHandler sirh = (ServerInfoReceiveHandler)ar.AsyncState;

                byte[] byPacket = sirh.udpclient.EndReceive(ar, ref sirh.ipepRemote);

                // Analyse the packet.
                BinaryReader br = new BinaryReader(new MemoryStream(byPacket));

                // Get the checksum.
                uint uiChecksum = br.ReadUInt32();

                // Skip ack and ackreturn and get packet type.
                br.ReadBytes(2);
                byte byPacketType = br.ReadByte();

                // Only interested in valid PT_SERVERINFO packets.
                if (byPacketType == PT_SERVERINFO && uiChecksum == SRB2Checksum(byPacket))
                {
                    bool bMalformed = true;

                    // Skip padding.
                    br.ReadByte();

                    // Remember where we are.
                    long iPacketStart = br.BaseStream.Position;

                    // Try to interpret the packet in each recognised format.
                    foreach (ServerInfoVer siv in Enum.GetValues(typeof(ServerInfoVer)))
                    {
                        SRB2ServerInfo srb2si;
                        byte byNumWads = 0;

                        srb2si.siv = siv;

                        br.BaseStream.Position = iPacketStart;

                        // Get address from socket.
                        srb2si.strAddress = sirh.ipepRemote.Address.ToString();
                        srb2si.unPort = unchecked((ushort)sirh.ipepRemote.Port);

                        // Get version.
                        byte byVersion = br.ReadByte();

                        if (siv == ServerInfoVer.SIV_PREME)
                        {
                            br.ReadBytes(3);

                            uint uiSubVersion = br.ReadUInt32();

                            // Format version.
                            // MattW_CFI: I hope you don't mind this exception, Oogaland, but 0.01.6 looks odd >_>
                            if (byVersion == 1 && uiSubVersion == 6)
                                srb2si.strVersion = "X.01.6";
                            else
                                srb2si.strVersion = byVersion.ToString();
                                //srb2si.strVersion = String.Format("{0}.{1:00}.{2}", byVersion / 100, byVersion % 100, uiSubVersion);
                        }
                        else
                        {
                            byte bySubVersion = br.ReadByte();

                            // Format version.
                            //srb2si.strVersion = String.Format("{0}.{1:00}.{2}", byVersion / 100, byVersion % 100, bySubVersion);
                            srb2si.strVersion = byVersion.ToString();
                        }

                        srb2si.byPlayers = br.ReadByte();
                        srb2si.byMaxplayers = br.ReadByte();
                        srb2si.byGametype = br.ReadByte();
                        srb2si.bModified = (br.ReadByte() != 0);

                        if (siv == ServerInfoVer.SIV_ME)
                            byNumWads = br.ReadByte();

                        srb2si.sbyAdminplayer = br.ReadSByte();

                        if (siv == ServerInfoVer.SIV_PREME)
                            br.ReadBytes(3);

                        // Calculate ping.
                        srb2si.uiTime = unchecked((uint)((long)(DateTime.Now.Ticks / 10000 - br.ReadUInt32()) % ((long)UInt32.MaxValue + 1)));

                        if (siv == ServerInfoVer.SIV_PREME)
                            br.ReadUInt32();

                        // Get and tidy map name.
                        if (siv == ServerInfoVer.SIV_PREME)
                        {
                            srb2si.strMapName = ReadFixedLengthStr(br, 8);
                            srb2si.strName = ReadFixedLengthStr(br, MAXSERVERNAME);
                        }
                        else
                        {
                            srb2si.strName = ReadFixedLengthStr(br, MAXSERVERNAME);
                            srb2si.strMapName = ReadFixedLengthStr(br, 8);
                        }

                        if (siv == ServerInfoVer.SIV_PREME)
                            byNumWads = br.ReadByte();

                        // Create new list of strings of initial size equal to number of wads.
                        srb2si.listFiles = new List<AddedWad>(byNumWads);

                        // Get the files info.
                        byte[] byFiles = br.ReadBytes(siv == ServerInfoVer.SIV_PREME ? 4096 : 936);
                        BinaryReader brFiles = new BinaryReader(new MemoryStream(byFiles));

                        // Extract the filenames.
                        try
                        {
                            for (int i = 0; i < byNumWads; i++)
                            {
                                bool bFullString = false;
                                AddedWad aw = new AddedWad();

                                if (siv == ServerInfoVer.SIV_PREME)
                                {
                                    aw.bImportant = brFiles.ReadByte() != 0;
                                    aw.downloadtype = (DownloadTypes)brFiles.ReadByte();
                                }
                                else
                                {
                                    byte byFileStatus = brFiles.ReadByte();
                                    aw.bImportant = (byFileStatus & 0xF) != 0;
                                    aw.downloadtype = (DownloadTypes)(byFileStatus >> 4);
                                }

                                aw.uiSize = brFiles.ReadUInt32();

                                // Work out how long the string is.
                                int iStringPos = (int)brFiles.BaseStream.Position;
                                while (iStringPos < byFiles.Length && byFiles[iStringPos] != 0) iStringPos++;

                                // Make sure it's not longer than the max name length.
                                if (iStringPos - (int)brFiles.BaseStream.Position > MAX_WADPATH)
                                {
                                    bFullString = true;
                                    iStringPos = MAX_WADPATH + (int)brFiles.BaseStream.Position;
                                }

                                // Get the info and add it, if possible.
                                if (iStringPos > (int)brFiles.BaseStream.Position)
                                {
                                    aw.strFilename = Encoding.ASCII.GetString(brFiles.ReadBytes(iStringPos - (int)brFiles.BaseStream.Position));
                                    srb2si.listFiles.Add(aw);
                                }

                                // Skip nul.
                                if (!bFullString) brFiles.ReadByte();

                                // Skip the md5sum.
                                brFiles.ReadBytes(16);
                            }

                            // Okay, done! Do something useful with the server info.
                            sirh.ProcessServerInfo(srb2si);

                            // If we got this far without an exception, leave the foreach loop.
                            bMalformed = false;
                            break;
                        }
                        catch (EndOfStreamException)
                        {
                            // Packet doesn't match supposed type, so we swallow the exception
                            // and try remaining types.
                        }
                        catch (Exception e)
                        {
                            sirh.HandleException(e);
                            break;
                        }
                    }

                    if (bMalformed)
                        sirh.HandleException(new Exception("Received invalid PT_SERVERINFO packet from " + sirh.ipepRemote.Address + ":" + sirh.ipepRemote.Port + "."));
                }

                // Resume listening.
                sirh.ipepRemote = new IPEndPoint(IPAddress.Any, 0);
                sirh.udpclient.BeginReceive(new AsyncCallback(Receive), sirh);
            }
        }

        public enum DownloadTypes
        {
            DT_TOOBIG = 0,
            DT_OK = 1,
            DT_DISABLED = 2
        }

        public struct AddedWad
        {
            public string strFilename;
            public bool bImportant;
            public uint uiSize;
            public DownloadTypes downloadtype;
        }

        public enum ServerInfoVer
        {
            SIV_PREME,
            SIV_ME
        };

        public struct SRB2ServerInfo
        {
            public string strAddress;
            public ushort unPort;

            public ServerInfoVer siv;

            public string strVersion;
            public byte byPlayers;
            public byte byMaxplayers;
            public byte byGametype;
            public bool bModified;
            public sbyte sbyAdminplayer;
            public uint uiTime;
            public string strMapName;
            public string strName;

            public List<AddedWad> listFiles;
        }
    }
}
