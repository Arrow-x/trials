#include "autocraft.hpp"

int main() {
#ifdef _WIN32
  enable_windows_ansi();
#endif
  Project my_project("EASTL_test");
  my_project.set_debug_messages(false);

  my_project.enable_file_logging(); // Logs to "build.log"

  my_project.auto_detect_sources("src");

  std::vector<Package> libs{
      {
          .name = "EABASE",
          .repo_url = "https://github.com/electronicarts/EABASE",
          .headers_path = {to_path({"libs", "EABASE", "include", "Common"})},
          .is_header_only = true,
          .async = false,
      },
      {
          .name = "EASTL",
          .repo_url = "https://github.com/electronicarts/EASTL/",
          .custom_build_cmd =
              "cd build/ && cmake .. -DCMAKE_BUILD_TYPE=Release "
              "-DCMAKE_POLICY_VERSION_MINIMUM=3.5 && "
              "cmake --build . -- -j 3",

          .async = false,
      }};
  my_project.add_library(libs);

  my_project.set_generating_compile_commands_json();
  my_project.close_log_file();
  int err = my_project.build("g++", "-g -O0");
  if (err == 0) {
    std::system("./autocraft_out/bin/EASTL_test");
  }

  return err;
}
