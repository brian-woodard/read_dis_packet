#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>

#include "json.hpp"

using json = nlohmann::json;

#include "SimUdpSocket.cpp"

struct TDisPdu
{
   double               TimeRelative;
   double               TimeDelta;
   std::vector<uint8_t> Pdu;
};

// Convert colon-separated hex string to byte buffer
// Example: "07:02:01:ff"
std::vector<uint8_t> hex_string_to_bytes(const std::string& hex)
{
   std::vector<uint8_t> bytes;
   bytes.reserve(hex.size() / 3);

   size_t i = 0;
   while (i < hex.size())
   {
      if (hex[i] == ':')
      {
         ++i;
         continue;
      }

      uint8_t value = static_cast<uint8_t>(
         std::stoul(hex.substr(i, 2), nullptr, 16)
      );
      bytes.push_back(value);
      i += 2;
   }

   return bytes;
}

int main(int argc, char* argv[])
{
   std::vector<TDisPdu> pdus;
   CSimUdpSocket        socket;
   double               first_time;

   if (argc != 2)
   {
      std::cerr << "Usage: " << argv[0] << " packets.json\n";
      return 1;
   }

   std::ifstream in(argv[1]);
   if (!in)
   {
      std::cerr << "Failed to open file\n";
      return 1;
   }

   json root;
   in >> root;

   if (!root.is_array())
   {
      std::cerr << "Expected top-level JSON array\n";
      return 1;
   }

   size_t packet_index = 0;

   for (const auto& pkt : root)
   {
      try
      {
         const std::string& payload_hex = pkt["_source"]["layers"]["udp"]["udp.payload"].get<std::string>();
         const double time_relative = std::strtod(pkt["_source"]["layers"]["udp"]["Timestamps"]["udp.time_relative"].get<std::string>().c_str(), nullptr);
         const double time_delta = std::strtod(pkt["_source"]["layers"]["udp"]["Timestamps"]["udp.time_delta"].get<std::string>().c_str(), nullptr);

         TDisPdu pdu;

         pdu.TimeRelative = time_relative;
         pdu.TimeDelta    = time_delta;
         pdu.Pdu          = hex_string_to_bytes(payload_hex);

         if (pdus.empty())
         {
            first_time = time_relative;
            pdu.TimeRelative = 0.0;
         }
         else
         {
            pdu.TimeRelative -= first_time;
         }

         // Replace angular velocity with (0.0, 0.0, -0.0857)
         size_t offset = 116;
         float ang_vel[3] = { 0.0f, 0.0f, -0.0857f };

         *(uint32_t*)&ang_vel[2] = htonl(*(uint32_t*)&ang_vel[2]);

         memcpy(&pdu.Pdu.data()[offset], ang_vel, sizeof(ang_vel));

         pdus.push_back(pdu);
      }
      catch (const std::exception& e)
      {
         std::cerr << "Error parsing packet " << packet_index
                   << ": " << e.what() << "\n";
      }
   }

   printf("Read %ld PDUs\n", pdus.size());

   // Dump first PDU to out.bin
   if (!pdus.empty())
   {
      size_t idx = pdus.size() - 1;
      printf("Total time: %f\n", pdus[idx].TimeRelative);

      std::ofstream out_file("out.bin", std::ios::binary);

      if (out_file.is_open())
      {
         out_file.write((const char*)pdus[0].Pdu.data(), pdus[0].Pdu.size());
         out_file.close();
      }
   }

   const char* ip_addr = "192.168.137.128";
   int port = 3000;
   printf("Opening UDP socket on %s:%d\n", ip_addr, port);
   socket.Open(ip_addr, port, port);

   size_t idx = 0;
   while (idx <= pdus.size())
   {
      if (idx == pdus.size())
      {
         printf("End of file, restarting...\n");
         idx = 0;
         usleep(10000);
      }
      else
      {
         int sleep_time = (int)(pdus[idx].TimeDelta * 1000000.0);
         usleep(sleep_time);
      }

      socket.SendToSocket((char*)pdus[idx].Pdu.data(), pdus[idx].Pdu.size());

      idx++;
   }

   return 0;
}

