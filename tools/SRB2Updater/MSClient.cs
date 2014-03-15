using System;
using System.Collections.Generic;
using System.Text;
using System.Net.Sockets;
using System.Net;
using System.IO;

namespace SRB2Updater
{
    class MSClient
    {
        private Socket socket;

        /// <summary>
        /// Constructs an MS client object.
        /// </summary>
        public MSClient()
        {
            socket = new Socket(
                AddressFamily.InterNetwork,
                SocketType.Stream,
                ProtocolType.IP);
        }

        /// <summary>
        /// Gets list of servers reported by the MS.
        /// </summary>
        /// <param name="strAddress">Hostname or IP address of MS.</param>
        /// <param name="unPort">Port of MS.</param>
        /// <returns>List of running servers.</returns>
        public List<MSServerEntry> GetServerList(string strAddress, ushort unPort)
        {
            // Resolve the address if necessary.
            IPHostEntry iphe = Dns.GetHostEntry(strAddress);

            int iIPIndex = 0;
            while (iIPIndex < iphe.AddressList.Length &&
                iphe.AddressList[iIPIndex].AddressFamily != socket.AddressFamily)
            {
                iIPIndex++;
            }

            // No addresses from our family?
            if (iIPIndex >= iphe.AddressList.Length)
                throw new SocketException((int)SocketError.HostNotFound);

            socket.Connect(iphe.AddressList[iIPIndex], unPort);

            // Send a request for the short server list.
            byte[] byPacket = new byte[12];
            BinaryWriter bw = new BinaryWriter(new MemoryStream(byPacket));
            bw.Write((uint)0);      // Unused
            bw.Write((uint)IPAddress.HostToNetworkOrder(205));    // GET_SHORT_SERVER_MSG
            bw.Write((uint)0);      // Length of tail.
            socket.Send(byPacket);

            List<MSServerEntry> listServers = new List<MSServerEntry>();

            // Don't sit and wait for ever.
            socket.ReceiveTimeout = 10000;

            // Keep reading packets. We break if we receive the sentinel end-packet.
            byte[] byServer = new byte[12 + 80];
            while (true)
            {
                int iLen = socket.Receive(byServer);
                BinaryReader br = new BinaryReader(new MemoryStream(byServer));

                // Ignore the first eight bytes.
                br.ReadInt64();

                // Is that the list finished?
                int iTailLen = IPAddress.NetworkToHostOrder(br.ReadInt32());
                if (iTailLen == 0)
                    break;
                else if (iTailLen != iLen - 12)
                {
                    // MS is in a bad mood.
                    //throw new Exception("Bad packet.");
                    break;
                }

                // Otherwise, add the server to the list.
                MSServerEntry msse = new MSServerEntry();

                br.ReadBytes(16);   // Skip.
                msse.strAddress = Encoding.ASCII.GetString(br.ReadBytes(16));

                string str = Encoding.ASCII.GetString(br.ReadBytes(8));
                int iPos = str.IndexOf("\0");
                if (iPos >= 0)
                    str = str.Remove(iPos);
                msse.unPort = Convert.ToUInt16(str);

                msse.strName = Encoding.ASCII.GetString(br.ReadBytes(32));
                iPos = msse.strName.IndexOf("\0");
                if (iPos >= 0)
                    msse.strName = msse.strName.Remove(iPos);

                msse.strVersion = Encoding.ASCII.GetString(br.ReadBytes(8));
                iPos = msse.strVersion.IndexOf("\0");
                if (iPos >= 0)
                    msse.strVersion = msse.strVersion.Remove(iPos);

                listServers.Add(msse);
            }

            return listServers;
        }
    }

    public struct MSServerEntry
    {
        public string strAddress;
        public ushort unPort;
        public string strName;
        public string strVersion;
        public bool bPermanent;
    }

}
