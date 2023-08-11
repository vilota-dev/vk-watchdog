#include "crow.h"
#include <sys/sysinfo.h>
#include <unordered_set>
#include <mutex>
#include <ecal/ecal.h>

std::string get_system_stats() {
  struct sysinfo sys_info;
  if (sysinfo(&sys_info) != 0) {
    throw std::runtime_error("sysinfo() failed!");
  }

  long uptime = sys_info.uptime;
  unsigned long total_ram = sys_info.totalram;
  unsigned long free_ram = sys_info.freeram;
  unsigned long used_ram = total_ram - free_ram;
  unsigned long cpu_loads_1min = sys_info.loads[0];
  unsigned long cpu_loads_5min = sys_info.loads[1];
  unsigned long cpu_loads_15min = sys_info.loads[2];

  return "Uptime: " + std::to_string(uptime) + "s\n" +
         "Total RAM: " + std::to_string(total_ram) + " bytes\n" +
         "Used RAM: " + std::to_string(used_ram) + " bytes\n" +
         "CPU Load (1 min): " + std::to_string(cpu_loads_1min) + "\n" +
         "CPU Load (5 min): " + std::to_string(cpu_loads_5min) + "\n" +
         "CPU Load (15 min): " + std::to_string(cpu_loads_15min) + "\n";
}

std::string exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

int main(int argc, char **argv) {
  auto status = eCAL::Initialize(0, nullptr, "vk-Watchdog", eCAL::Init::Default | eCAL::Init::Monitoring);
  if (status == -1) CROW_LOG_ERROR << "Failed to initialize eCAL";
  eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "Running");
  eCAL::Monitoring::SetFilterState(false);

  crow::SimpleApp app;
  std::mutex mtx;
  std::unordered_set<crow::websocket::connection*> users;

  CROW_ROUTE(app, "/")([](){
    char name[256];
    gethostname(name, 256);
    crow::mustache::context x;
    x["servername"] = name;

    auto page = crow::mustache::load("ws.html");
    return page.render(x);
  });

  CROW_ROUTE(app, "/lsusb")([](){
    return exec("lsusb");
  });

  CROW_ROUTE(app, "/system_stats")([](){
    return get_system_stats();
  });

  CROW_ROUTE(app, "/ws").websocket()
          .onopen([&](crow::websocket::connection& conn) {
            CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
            std::lock_guard<std::mutex> _(mtx);
            users.insert(&conn);
          })
          .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            CROW_LOG_INFO << "websocket connection closed: " << reason;
            std::lock_guard<std::mutex> _(mtx);
            users.erase(&conn);
          })
          .onmessage([&](crow::websocket::connection& /*conn*/, const std::string& data, bool is_binary) {
            if (is_binary) {
              // Binary messages are ignored
              return;
            }

            // Execute the command
            std::array<char, 128> buffer;
            std::string result;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(data.c_str(), "r"), pclose);
            if (!pipe) {
              result = "Error executing command.";
            } else {
              while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
              }
            }

            // Send the result back to all connected users
            std::lock_guard<std::mutex> _(mtx);
            for (auto u : users)
              u->send_text(result);
          });

  CROW_ROUTE(app, "/upload_file")
          .methods(crow::HTTPMethod::Post)([](const crow::request& req) {
            crow::multipart::message file_message(req);
            for (const auto& part : file_message.part_map)
            {
              const auto& part_name = part.first;
              const auto& part_value = part.second;
              CROW_LOG_DEBUG << "Part: " << part_name;
              if ("InputFile" == part_name)
              {
                // Extract the file name
                auto headers_it = part_value.headers.find("Content-Disposition");
                if (headers_it == part_value.headers.end())
                {
                  CROW_LOG_ERROR << "No Content-Disposition found";
                  return crow::response(400);
                }
                auto params_it = headers_it->second.params.find("filename");
                if (params_it == headers_it->second.params.end())
                {
                  CROW_LOG_ERROR << "Part with name \"InputFile\" should have a file";
                  return crow::response(400);
                }
                const std::string outfile_name = params_it->second;

                for (const auto& part_header : part_value.headers)
                {
                  const auto& part_header_name = part_header.first;
                  const auto& part_header_val = part_header.second;
                  CROW_LOG_DEBUG << "Header: " << part_header_name << '=' << part_header_val.value;
                  for (const auto& param : part_header_val.params)
                  {
                    const auto& param_key = param.first;
                    const auto& param_val = param.second;
                    CROW_LOG_DEBUG << " Param: " << param_key << ',' << param_val;
                  }
                }

                // Create a new file with the extracted file name and write file contents to it
                std::ofstream out_file(outfile_name);
                if (!out_file)
                {
                  CROW_LOG_ERROR << " Write to file failed\n";
                  continue;
                }
                out_file << part_value.body;
                out_file.close();
                CROW_LOG_INFO << " Contents written to " << outfile_name << '\n';
              }
              else
              {
                CROW_LOG_DEBUG << " Value: " << part_value.body << '\n';
              }
            }
            return crow::response(200);
          });

  CROW_ROUTE(app, "/upload_json")
          .methods(crow::HTTPMethod::Post)([](const crow::request& req) {
            auto x = crow::json::load(req.body);
            if (!x) {
              return crow::response(400);
            }
            CROW_LOG_INFO << "Received JSON: " << x;
            return crow::response(200, "JSON received successfully");
          });


  //set the port, set the app to run on multiple threads, and run the app
  app.port(18080).multithreaded().run();

  eCAL::Finalize();
}
