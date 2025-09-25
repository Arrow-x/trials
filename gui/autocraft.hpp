#ifndef AUTOCRAFT_HPP
#define AUTOCRAFT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <sstream>
#include <thread>
#include <vector>

// TODO: add a fucntion to compile into a shared or static library, and output
// into autocraft_out/lib

// TODO: add a fucntion to export headers, and output
//  into autocraft_out/include

// TODO: add a way for the user to set a thread number to the add_library
// function

// TODO: add unit testing

// TODO: add support to vcpkg as a first class packages source

// TODO: change the ansi codes to use the terminal themes color

// TODO: refactor the build() function to be more generic

// TODO: provide better default flags

// TODO: add a modes enum (debug, library ..etc)??

namespace fs = std::filesystem;

enum Supported {
  AUTOCRAFT,
  CMAKE,
  MAKE,
  MESON,
  ZIG,
  SCONS,
  AUTOTOOLS,
  PREMAKE,
  CUSTOM,
  UNSET
};

// The representation of the package that you want the system to fetch and build
// for you
struct Package {
  // The name of the package that is going to be used for naming the
  // directory, and default name for linking, if you want to use this for
  // anything else please set the Package.artifact_name
  std::string name;
  // A URL link to a git repo
  std::string repo_url;
  // Specify the tag that you want checkout, this setting this will override
  // commit and branch
  std::string tag;
  // Specify the commit that you want checkout, this setting will be
  // overridden by tag, and will override branch
  std::string commit;
  // Specify the active branch that you want to switch to, this setting will
  // be overridden by both tag and commit, the branch will pull on each build
  std::string branch;
  // Enter the full compile cmd that you want, make sure to not set
  // preferred_build_system to anything else though
  std::string custom_build_cmd;
  // Pass in the environment variables you want set before compilation
  std::map<std::string, std::string> env_vars;
  // An enum of all the pre-specified compile options, Supported::UNSET will
  // use the one that you provide in custom_build_cmd
  Supported preferred_build_system = Supported::UNSET;
  // A path (to_path function is provided) to where the package build
  // system store the result of the compilation
  fs::path artifact_dir;
  // The name of the result of the compilation, not setting this the system
  // will use `name` instead
  fs::path artifact_name;
  // A vector of paths (to_path function is provided) to the headers directory
  // that you want to include in your project (disables the autodetection)
  std::vector<fs::path> headers_path;
  // A vector of paths (to_path function is provided) to headers directory
  // that you want to include in your project while piggybacking on the
  // autodetection (include both)
  std::vector<fs::path> additional_headders_path;
  // A bool to skipping processing this Package, this will not be fetched nor
  // built nor linked nor included
  bool enabled = true;
  // A bool to force a rebuild and ignore the cache
  bool force = false;
  // A bool to skip rebuilding this Package even if the cache is invalidated
  bool is_precompiled_binary = false;
  // Tell the system that this package is header only
  bool is_header_only = false;
  // Disbale running the process of managing this package into a seperate
  // thread and force it into the main one instead
  bool async = true;

  bool skip_system_lib = false;
};

inline fs::path to_path(const std::vector<std::string> &parts) {
  fs::path result;
  for (const auto &part : parts) {
    result /= part; // Use /= to ensure proper path concatenation
  }
  return result;
}

class Project {
public:
  Project(const std::string &name) : project_name(name) {}

  // Add a source file to be compiled, if the second argument is true, then it
  // will clear all the sources you set previously
  void add_source(const std::string &src, bool clear = false) {
    if (clear) {
      std::string _log;
      for (auto &s : sources) {
        _log += s;
      }
      log(LogLevel::DEBUG, "clearing all of theses added sources: " + _log);
      sources.clear();
    }
    sources.emplace_back(src);
  }

  // Pass a string with the the std version to use defaults to (don't enclude
  // the dash) defaults to c++17
  void set_std_version(const std::string &v) { std_version = v; }

  void set_debug_messages(bool b) { debug_messages = b; }

  // A path to where you want your project compilation resulted binary
  // is stored
  // void set_artifacts_dir(const std::string &dir) { artifacts_dir = dir; }

  // The name of your project compilation resulted binary
  // void set_artifacts_name(const std::string &name) { artifacts_name = name;
  // }

  // A path (to_path function is provided) to where you want the
  // Packages (and the .built cache directory) to be stored within this
  // project directory
  void set_libs_directory(std::string &libs_dir) { libs_dir = libs_dir; }

  // Generate a compile_commands.json for the project (to play nice with
  // clangd)
  void set_generating_compile_commands_json() { generate_json = true; }

  // Remove build artifacts and cached dependencies (defaults to libs/.built)
  void clean() {
    if (fs::exists(artifacts_dir)) {
      auto _err = fs::remove_all(artifacts_dir);
      if (_err == 0) {
        log(LogLevel::WARNING,
            "didn't remove anything from the artifacts directory at: " +
                artifacts_dir);
      } else {
        log(LogLevel::DEBUG, "Removed build artifacts in " + artifacts_dir);
      }
    }

    fs::path cache_dir = to_path({libs_dir, ".built"});
    if (fs::exists(cache_dir)) {
      auto _err = fs::remove_all(cache_dir);
      if (_err == 0) {
        log(LogLevel::WARNING,
            "didn't remove anything from the cache in " + cache_dir.string());
      } else {
        log(LogLevel::DEBUG,
            "Removed cached dependencies in " + cache_dir.string());
      }
    }
  }

  void enable_file_logging(const std::string &filename = "build.log") {
    log_file.open(filename, std::ios::app);
    if (!log_file) {
      log(LogLevel::ERROR, "Failed to open log file: " + filename);
      log_to_file = false;
    } else {
      log_to_file = true;
      log(LogLevel::DEBUG, "Logging to file: " + filename);
    }
  }

  void close_log_file() { log_file.close(); }

  /**
   * Set the dependencies for the project so the build system can fetch and
   * build them.
   *
   * @param package_db The vector of `Package` to fetch and build.
   * @param clear_flags If `true`, clears the includes and linked libraries
   * from the last build. Defaults to `false`.
   * @param clear remove all the packages that are on disk but not included in
   * the `package_db`
   */
  void add_library(std::vector<Package> &package_db,
                   const bool clear_flags = false, bool clear = true) {
    if (clear_flags) {
      include_headers.clear();
      linked_libraries.clear();
    }
    if (clear) {
      sync_packages_db(package_db);
    }

    std::vector<std::thread> async_threads;
    // TODO: the loggin (both my own and stdout of the cmd that the sytem
    // calls are all over eachohter figure out a way to fix it)

    for (auto &pkg : package_db) {
      if (!pkg.enabled) {
        continue;
      }
      if (pkg.async) {
        async_threads.emplace_back(fetch_and_build_package(pkg));
      } else {
#ifdef _WIN32
        if (fetch_package(pkg)) {
          build_package(pkg);
          cache_package(pkg);
        }
#else
        if (pkg.skip_system_lib || add_pkg_info_package(pkg.name) != 0) {

          if (fetch_package(pkg)) {
            build_package(pkg);
            cache_package(pkg);
          }
        };
#endif
      }
    }
    // Wait for all async threads
    for (auto &t : async_threads) {
      if (t.joinable())
        t.join();
    }
  }

#ifdef _WIN32
#else
  int add_pkg_info_package(const std::string &pkg_name) {
    if (!is_tool_available("pkg-config")) {
      return 1;
    }
    if (run_command("pkg-config --exists " + pkg_name) != 0) {
      return 1;
    }
    log(LogLevel::INFO, "Using the system wide installed lib for " + pkg_name);

    std::string libs_L =
        run_command_and_get_output("pkg-config --libs-only-L " + pkg_name) +
        " ";
    std::string libs_l =
        run_command_and_get_output("pkg-config --libs-only-l " + pkg_name) +
        " ";

    std::string cflags =
        run_command_and_get_output("pkg-config --cflags-only-I " + pkg_name) +
        " ";

    std::string other_flags =
        run_command_and_get_output("pkg-config --cflags-only-other " +
                                   pkg_name) +
        " ";

    libs_l +=
        run_command_and_get_output("pkg-config --libs-only-other " + pkg_name) +
        " ";

    for (auto &s : split(cflags, " ")) {
      if (!s.empty()) {
        append_include_headers({s});
      }
    }
    for (auto &s : split(libs_L, " ")) {
      if (!s.empty()) {
        pkg_config_link_flags.emplace_back(s);
      }
    }
    for (auto &s : split(libs_l, " ")) {
      if (!s.empty()) {
        pkg_config_link_flags.emplace_back(s);
      }
    }
    for (auto &s : split(other_flags, " ")) {
      if (!s.empty()) {
        pkg_config_other_flags.emplace_back(s);
      }
    }
    return 0;
  }
#endif

  // Pass a directory and the build system will auto detect all .cpp files in
  // it
  void auto_detect_sources(const std::string &src_dir) {
    for (const auto &entry :
         fs::recursive_directory_iterator(fs::path(src_dir))) {
      if (entry.path().extension() == ".cpp") {
        sources.emplace_back(entry.path().string());
      }
    }
  }

  void enable_dry_run() { dry_run = true; }

  bool is_tool_available(const std::string &tool) {
#ifdef _WIN32
    std::string cmd = "where " + tool + " >nul 2>&1";
#else
    std::string cmd = "which " + tool + " >/dev/null 2>&1";
#endif
    if (std::system(cmd.c_str()) != 0) {
      log(ERROR, tool + " is not installed!");
      return false;
    }
    return true;
  }

  /**
   * Pass in a a vector of strings of the the Headers that you want to
   * include, don't put the flag just the path you need
   **/
  void append_include_headers(const std::vector<fs::path> &paths) {
    for (auto &p : paths) {
      include_headers.emplace_back(p);
    }
  }

  /**
   * Pass in a vector of pairs of std::strings to pass linker arguments (path,
   *then name) to the compiler don't add the flags just the names
   **/
  void append_link_flags(
      const std::vector<std::pair<std::string, std::string>> &pairs) {
    for (auto &p : pairs) {
      user_link_flags.emplace_back(p);
    }
  }

  // Start building
  int build(const std::string &c_cmd, const std::string &compiler_flags) {
    if (!is_tool_available(c_cmd)) {
      log(ERROR, "This compiler: " + c_cmd + " isn't properly installed!");
      return 1;
    }
    project_compiler = c_cmd; // for some logging?

    std::string compile_cmd = c_cmd + " " + compiler_flags + " ";
    if (project_compiler == "cl") {
      compile_cmd += "/std=" + std_version + " ";
    } else {

      compile_cmd += "-std=" + std_version + " ";
    }

    // Include headers
    for (auto &include : include_headers) {
      if (include.empty()) {
        continue;
      }
      if (project_compiler == "cl") {
        compile_cmd += "/I" + include.string() + " ";
      } else {
        compile_cmd += "-I" + include.string() + " ";
      }
    }

#ifdef _WIN32
#else
    for (auto &c : pkg_config_other_flags) {
      if (!c.empty()) {
        compile_cmd += c + " ";
      }
    }
#endif

    // Add source files
    for (const auto &src : sources) {
      compile_cmd += src + " ";
    }

    // Create artifacts directory if it doesn't exist
    if (!fs::exists(fs::path(artifacts_dir))) {
      auto err = fs::create_directories(artifacts_dir);
      if (!err) {
        log(LogLevel::ERROR, "Couldn't create the artifacts directory");
      }
    }

    // Set output executable
    if (project_compiler == "cl") {
      compile_cmd +=
          "/Fe" + to_path({artifacts_dir, artifacts_name}).string() + " ";
    } else {
      compile_cmd +=
          "-o " + to_path({artifacts_dir, artifacts_name}).string() + " ";
    }

    // Link libraries
    for (const auto &pkg : linked_libraries) {
      if (project_compiler == "cl") {
        compile_cmd += "/link /LIBPATH:" + pkg.artifact_dir.string() + " " +
                       pkg.artifact_name.string() + ".lib " + " ";
      } else {
        compile_cmd += "-L" + pkg.artifact_dir.string() + " -l" +
                       pkg.artifact_name.string() + " ";
      }
    }

    for (const auto &[lib_path, lib_name] : user_link_flags) {
      if (project_compiler == "cl") {
        compile_cmd += "/link /LIBPATH:" + lib_path + " " + lib_name + ".lib ";
      } else {
        compile_cmd += "-L" + lib_path + " -l" + lib_name + " ";
      }
    }

#ifdef _WIN32
#else
    for (auto &l : pkg_config_link_flags) {
      if (!l.empty()) {
        compile_cmd += l + " ";
      }
    }
#endif

    log(LogLevel::INFO,
        "[Building] " + project_name + " using " + project_compiler);

    int err = run_command(compile_cmd);

    if (generate_json) {
      generate_compile_json(compile_cmd);
    }

    return err;
  }

private:
  std::string project_name;
  std::string project_compiler;
  std::vector<std::string> sources;
  std::vector<Package> linked_libraries;
  std::vector<std::pair<std::string, std::string>> user_link_flags;
  std::vector<std::string> pkg_config_link_flags;
  std::vector<std::string> pkg_config_other_flags;
  std::string std_version = "c++2a";
  fs::path libs_dir = "libs";
  std::string artifacts_dir = to_path({"autocraft_out", "bin"});
  std::string artifacts_name = project_name;
  std::vector<fs::path> include_headers;
  std::ofstream log_file;
  bool generate_json = false;
  bool log_to_file = false;
  bool dry_run = false;
  bool debug_messages = false;

#ifdef _WIN32
  std::string separator = " & "; // Works in CMD
#else
  std::string separator = " && "; // Works in Unix shells
#endif

  enum LogLevel { INFO, WARNING, ERROR, DEBUG, NONE };

  std::vector<std::string> split(const std::string &str,
                                 std::string delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
      tokens.emplace_back(str.substr(start, end - start));
      start = end + 1;
    }
    tokens.emplace_back(str.substr(start)); // Last token

    return tokens;
  }

  std::thread fetch_and_build_package(Package &pkg) {
    return std::thread([&]() {
#ifdef _WIN32
      if (fetch_package(pkg)) {
        build_package(pkg);
        cache_package(pkg);
      }
#else
      if (pkg.skip_system_lib || add_pkg_info_package(pkg.name) != 0) {
        if (fetch_package(pkg)) {
          build_package(pkg);
          cache_package(pkg);
        };
      };
#endif
    });
  }

  std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "[%H:%M:%S] ");
    return ss.str();
  }

  std::string vector_of_paths_to_string(std::vector<fs::path> &vec) {
    std::string r;
    for (auto &s : vec) {
      r += s.string();
    }
    return r;
  }

  void log(LogLevel level, const std::string &message) {
    std::string prefix = get_timestamp();
    std::ostream &out = (level == ERROR) ? std::cerr : std::cout;

    std::string RESET = "\033[0m";
    std::string RED = "\033[31m";
    std::string YELLOW = "\033[33m";
    std::string GREEN = "\033[32m";
    std::string BLUE = "\033[34m";

    switch (level) {
    case INFO:
      out << prefix;
      out << GREEN << "[INFO] " << message << RESET << std::endl;
      if (log_to_file)
        log_file << prefix << "[INFO] " << message << std::endl;
      break;
    case WARNING:
      out << prefix;
      out << YELLOW << "[WARNING] " << message << RESET << std::endl;
      if (log_to_file)
        log_file << prefix << "[WARNING] " << message << std::endl;
      break;
    case ERROR:
      out << prefix;
      out << RED << "[ERROR] " << message << RESET << std::endl;
      if (log_to_file)
        log_file << prefix << "[ERROR] " << message << std::endl;
      break;
    case DEBUG:
      if (log_to_file)
        log_file << prefix << "[DEBUG] " << message << std::endl;
      if (debug_messages) {
        out << prefix;
        out << BLUE << "[DEBUG] " << message << RESET << std::endl;
      }
      break;
    case NONE:
      break;
    }

    if (log_to_file)
      log_file.flush();
  }

  int run_command(const std::string &cmd) {
    log(LogLevel::DEBUG, "[RUN] " + cmd);

    if (dry_run)
      return 0; // Simulate success

    int result = std::system(cmd.c_str());

    if (result != 0) {
      log(LogLevel::DEBUG,
          "Command failed (exit code " + std::to_string(result) + "): " + cmd);
      return result;
    }
    return 0;
  }

  void sync_packages_db(std::vector<Package> &package_db) {
    fs::path libs_path = fs::path(libs_dir);

    if (!fs::exists(libs_path)) {
      log(LogLevel::DEBUG, "libs folder does not exist. Nothing to sync.");
      return;
    }

    // Iterate over the libs/ folder
    for (const auto &entry : fs::directory_iterator(libs_path)) {
      if (entry.is_directory()) {
        std::string package_name = entry.path().filename().string();

        // Skip the .built folder
        if (package_name == ".built") {
          continue;
        }

        // Check if the package is in the package_db
        bool is_used = false;
        for (const auto &pkg : package_db) {
          if (package_name == pkg.name) {
            is_used = true;
            break; // Stop searching once we find a match
          }
        }

        // If the package is not used, delete it
        if (!is_used) {
          log(LogLevel::DEBUG, "Removing unused package: " + package_name);
          fs::remove_all(entry.path()); // Delete the unused package
          fs::remove_all(to_path(
              {libs_path, ".built", package_name})); // remove the corresponding
                                                     // file in .built as well
        }
      }
    }
  }

  std::string serialize_package(const Package &pkg) {
    std::stringstream ss;
    ss << pkg.name << "|" << pkg.repo_url << "|" << pkg.branch << "|"
       << pkg.commit << "|" << pkg.tag << "|" << pkg.custom_build_cmd << "|"
       << "|" << pkg.preferred_build_system << "|" << pkg.is_precompiled_binary
       << "|" << pkg.force << "|" << pkg.is_header_only << "|";

    for (const auto &[key, val] : pkg.env_vars) {
      ss << "|" << key << "|" << val;
    }

    return ss.str();
  }

  int retry_command(int attempts, std::string &cmd, LogLevel log_level,
                    const std::string &msg) {
    int err;
    while (attempts > 0) {
      err = run_command(cmd);
      if (err != 0) {
        log(log_level, "[" + std::to_string(attempts) + "] " + msg);
        // Exit early on fatal errors (e.g., wrong repo URL)
        if (err == 128) {
          log(LogLevel::ERROR, "Fatal error detected. Exiting retries.");
          return err;
        }
        std::this_thread::sleep_for(
            std::chrono::seconds(1)); // Small delay before retry
        attempts--;
      } else {
        return 0;
      }
    }
    return err;
  }

  std::string run_command_and_get_output(const std::string &cmd) {
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      log(LogLevel::ERROR, "Failed to run: " + cmd);
      return "";
    }

    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
    }
    pclose(pipe);

    // Trim trailing newlines
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    log(LogLevel::DEBUG, "Ran command: " + cmd + " → Output: " + result);
    return result;
  }

  // Helper function to simplify Git commands
  std::string git_cmd(const fs::path &repo, const std::string &cmd) {
    return "cd " + repo.string() + " && git " + cmd;
  }

  bool fetch_package(Package &pkg) {
    if (!is_tool_available("git")) {
      return 1;
    }
    fs::path path = fs::path(libs_dir) / pkg.name;

    if (!fs::exists(path)) {
      if (pkg.repo_url.empty()) {
        log(LogLevel::ERROR, "No repo URL provided for package: " + pkg.name);
        return false;
      }

      log(LogLevel::INFO,
          "Fetching " + pkg.name + " from " + pkg.repo_url + "...");
      std::string clone_cmd =
          "git clone --progress " + pkg.repo_url + " " + path.string();
      std::string err_msg = "Failed to fetch package: " + pkg.name;

      if (retry_command(5, clone_cmd, LogLevel::WARNING, err_msg) != 0) {
        log(LogLevel::ERROR,
            "Failed to fetch " + pkg.name + " from " + pkg.repo_url);
        return false;
      }
    }

    // Handle tag resolution
    if (!pkg.tag.empty()) {
      log(LogLevel::DEBUG, "Fetching commit hash for tag: " + pkg.tag);
      pkg.commit =
          run_command_and_get_output(git_cmd(path, "rev-parse " + pkg.tag));
    }

    // Check if already fetched
    if (!pkg.commit.empty() && is_package_fetched(pkg.name, pkg.commit)) {
      log(LogLevel::DEBUG,
          pkg.name + " already fetched at commit: " + pkg.commit);
      return true;
    }

    // Detect the default branch
    if (run_command(git_cmd(path, "rev-parse --verify refs/heads/main")) == 0) {
      run_command(git_cmd(path, "switch main"));
    } else if (run_command(git_cmd(
                   path, "rev-parse --verify refs/heads/master")) == 0) {
      run_command(git_cmd(path, "switch master"));
    } else {
      std::string default_branch = run_command_and_get_output(
          git_cmd(path, "symbolic-ref refs/remotes/origin/HEAD | sed "
                        "'s@^refs/remotes/origin/@@'"));
      if (!default_branch.empty()) {
        run_command(git_cmd(path, "switch " + default_branch));
      }
    }

    // Try pulling latest changes
    if (run_command(git_cmd(path, "pull")) != 0) {
      log(LogLevel::WARNING,
          "Git pull failed for " + pkg.name + ". Possible network issue?");
    }

    // Checkout requested version
    std::string checkout_cmd;
    if (!pkg.tag.empty()) {
      log(LogLevel::DEBUG, "Checking out tag: " + pkg.tag);
      checkout_cmd = git_cmd(path, "checkout " + pkg.tag);
    } else if (!pkg.commit.empty()) {
      log(LogLevel::DEBUG, "Checking out commit: " + pkg.commit);
      checkout_cmd = git_cmd(path, "checkout " + pkg.commit);
    } else if (!pkg.branch.empty()) {
      log(LogLevel::DEBUG, "Switching to branch: " + pkg.branch);
      checkout_cmd = git_cmd(path, "switch " + pkg.branch);
    }

    if (run_command(checkout_cmd) != 0) {
      log(LogLevel::ERROR,
          "Failed to checkout requested version for " + pkg.name);
      return false;
    }

    // Clear cache if the commit changed
    if (pkg.commit.empty()) {
      auto dir_path = to_path({libs_dir, ".built" + pkg.name});
      if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
        for (const auto &entry : fs::directory_iterator(dir_path)) {
          fs::remove_all(entry.path());
        }
      }
      pkg.commit = run_command_and_get_output(git_cmd(path, "rev-parse HEAD"));
    }

    return true;
  }

  // Function to read the content of a file
  std::optional<std::string> read_file(const fs::path &file_path) {
    std::ifstream file(file_path);
    if (!file) {
      log(LogLevel::ERROR, "Failed to open file: " + file_path.string());
      return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  void write_hash_file(const fs::path &file_path, std::uint32_t hash_value) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
      log(LogLevel::ERROR,
          "Failed to open file for writing: " + file_path.string());
      return;
    }
    file.write(reinterpret_cast<const char *>(&hash_value), sizeof(hash_value));
  }

  std::optional<std::uint64_t> read_hash_file(const fs::path &file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      log(LogLevel::ERROR, "Failed to open file: " + file_path.string());
      return std::nullopt;
    }

    std::uint64_t hash_value;
    // file.read(reinterpret_cast<char *>(&hash_value), sizeof(hash_value));
    file >> hash_value;

    if (!file) {
      log(LogLevel::ERROR,
          "Failed to read hash from file: " + file_path.string());
      return std::nullopt;
    }

    return hash_value;
  }

  bool is_cache_valid(const Package &pkg) {
    fs::path path = to_path({libs_dir, pkg.name});
    fs::path marker_path =
        to_path({path.parent_path(), ".built", pkg.name, pkg.commit});

    if (!fs::exists(marker_path)) {
      log(LogLevel::ERROR, "Couldn't find the marker file for: " + pkg.name);
      return false; // Cache does not exist
    }

    // Read the stored hash from the file
    uint64_t stored_hash;
    auto r = read_hash_file(marker_path);
    if (!r) {
      log(LogLevel::ERROR,
          "Couldn't read the checksum from the marker file at: " +
              marker_path.string());
      return false;
    } else {
      stored_hash = *r;
    }

    // Compute the new hash and compare as integers
    uint64_t current_hash = hash_package(serialize_package(pkg));

    log(LogLevel::DEBUG,
        "Stored hash: " + std::to_string(stored_hash) +
            ", Computed hash: " + std::to_string(current_hash));
    return stored_hash == current_hash;
  }

  bool is_package_cached(const Package &pkg) {
    if (pkg.force) {
      log(LogLevel::INFO, pkg.name + " is forcing the build process!");
      return false;
    }
    fs::path lib_path = to_path({libs_dir, ".built", pkg.name, pkg.commit});

    if (fs::exists(lib_path.string())) {
      if (is_cache_valid(pkg)) {
        return true;
      }
    };
    return false;
  }

  bool is_package_fetched(std::string &pkg_name, std::string &pkg_commit) {
    fs::path lib_path = to_path({libs_dir, ".built", pkg_name, pkg_commit});
    if (fs::exists(lib_path)) {
      return true;
    }
    return false;
  }

  std::string
  build_command_with_env(const std::map<std::string, std::string> &env_vars,
                         const std::string &build_cmd) {
    std::string cmd;
#ifdef _WIN32
    // On Windows, use 'set VAR=VALUE &&' to set environment variables for a
    // single command
    for (const auto &[key, value] : env_vars) {
      cmd += "set " + key + "=" + value + " && ";
    }
    cmd += build_cmd;
#else
    // On Unix-like systems, use 'VAR=VALUE' syntax before the command
    for (const auto &[key, value] : env_vars) {
      cmd += key + "=" + value + " ";
    }
    cmd += build_cmd;
#endif

    return cmd;
  }

  // Detects the build system from the package directory
  Supported find_build_system(const fs::path &path) {
    if (fs::exists(path / "autocraft.cpp"))
      return Supported::AUTOCRAFT;
    if (fs::exists(path / "build.zig"))
      return Supported::ZIG;
    if (fs::exists(path / "CMakeLists.txt"))
      return Supported::CMAKE;
    if (fs::exists(path / "Makefile"))
      return Supported::MAKE;
    if (fs::exists(path / "meson.build"))
      return Supported::MESON;
    if (fs::exists(path / "SConstruct"))
      return Supported::SCONS;
    if (fs::exists(path / "configure"))
      return Supported::AUTOTOOLS;
    if (fs::exists(path / "premake5.lua"))
      return Supported::PREMAKE;
    return Supported::UNSET;
  }

  std::string build_cmd(fs::path path, std::string cmd) {
    return "cd " + path.string() + separator + cmd;
  }

  std::string get_build_command(Supported system, const fs::path &path) {
    auto l_threads = std::to_string(std::thread::hardware_concurrency());

    switch (system) {
    case Supported::ZIG:
      return build_cmd(path, "zig build");
    case Supported::CMAKE:
      fs::create_directories(path / "build");
      return build_cmd(path / "build",
                       "cmake .. -DCMAKE_BUILD_TYPE=Release" + separator +
                           "cmake --build . -- -j " + l_threads);
      // Gets the appropriate build command for the detected build system
    case Supported::AUTOCRAFT:
      // TODO: this will not run the bin produced, and hardcodding g++
      // might not work on windows, I might need to just bite the bullet
      // and have a manifast file
      return build_cmd(path, "g++ autocraft.cpp -o autocraft && ./autocraft");
    case Supported::MAKE:
      return build_cmd(path, "make -j" + l_threads);
    case Supported::MESON:
      return build_cmd(path,
                       "meson setup build" + separator + "ninja -C build");
    case Supported::SCONS:
      return build_cmd(path, "scons -j" + l_threads);
    case Supported::AUTOTOOLS:
      return build_cmd(path, "./configure" + separator + "make");
    case Supported::PREMAKE:
      return build_cmd(path, "premake5 gmake" + separator + "make");
    case Supported::CUSTOM:
      return ""; // Custom command handled separately
    default:
      return "";
    }
  }

  // Sets default artifact paths if none are provided
  void detect_artifact_paths(Package &pkg, const fs::path &path) {
    if (!pkg.artifact_dir.empty()) {
      return;
    }
    std::vector<fs::path> common_artifacts_dirs = {
        to_path({"zig-out", "lib"}), "build", to_path({"build", "lib"}), "lib",
        pkg.name};
    for (const auto &dir : common_artifacts_dirs) {
      if (fs::exists(path / dir)) {
        pkg.artifact_dir = path / dir;
        log(LogLevel::DEBUG, "Detected common artifacts buils location in: " +
                                 (path / dir).string());
        return;
      }
    }
    log(LogLevel::DEBUG,
        "Common artifacts builds localtion not found, please provide a "
        "path in Package.artifact_dir, defaulting to repo root.");
    pkg.artifact_dir = path;
  }

  void detect_artifact_names(Package &pkg, const fs::path &path) {
    if (!pkg.artifact_name.empty()) {
      return;
    }
    pkg.artifact_name = pkg.name;
    log(LogLevel::DEBUG, "No compiled library file name provided, "
                         "defaulting to to Package.name that you set " +
                             pkg.artifact_name.string());
  }

  Supported detect_build_system(Package &pkg, fs::path &path) {
    Supported res = find_build_system(path);
    if (res != Supported::UNSET) {
      log(LogLevel::INFO, "Detected build system: " + supported_to_string(res) +
                              " for " + pkg.name);
    }
    return res;
  }

  std::string supported_to_string(Supported s) {
    switch (s) {
    case AUTOCRAFT:
      return "autocraft.cpp";
    case CMAKE:
      return "cmake";
    case MAKE:
      return "make";
    case MESON:
      return "meson";
    case ZIG:
      return "zig";
    case SCONS:
      return "scons";
    case AUTOTOOLS:
      return "gnu autotools";
    case PREMAKE:
      return "premake";
    case CUSTOM:
      return "user-set";
    case UNSET:
      return "unknown";
    default:
      return "Invalid";
    }
  }

  void detect_headers(Package &pkg, const fs::path &path) {
    if (!pkg.headers_path.empty())
      return;

    std::vector<fs::path> common_header_dirs = {
        to_path({"zig-out", "include"}), "include",
        to_path({"build", "include"}), pkg.name, "src"};

    for (const auto &dir : common_header_dirs) {
      if (fs::exists(path / dir)) {
        pkg.headers_path.emplace_back(path / dir);
        log(LogLevel::DEBUG, "Detected headers in: " + (path / dir).string());
        return;
      }
    }

    log(LogLevel::DEBUG, "Headers not found, defaulting to repo root.");
    pkg.headers_path.emplace_back(path);
  }

  // Attempts to build using a command
  bool try_build(const Package &pkg, const std::string &build_cmd) {
    if (build_cmd.empty())
      return false;
    log(LogLevel::DEBUG, "[Building] Running: " + build_cmd);
    return (run_command(build_cmd) == 0);
  }

  // Tries all possible build systems until one succeeds
  Supported try_build_systems(const Package &pkg, const fs::path &path) {
    Supported build_systems[] = {Supported::ZIG,      Supported::CMAKE,
                                 Supported::MAKE,     Supported::MESON,
                                 Supported::SCONS,    Supported::PREMAKE,
                                 Supported::AUTOTOOLS};

    log(DEBUG, supported_to_string(pkg.preferred_build_system) +
                   " is being attempted");
    if (is_build_system_installed(pkg.preferred_build_system)) {
      if (try_build(pkg, get_build_command(pkg.preferred_build_system, path))) {
        return pkg.preferred_build_system;
      }
    }
    log(DEBUG,
        supported_to_string(pkg.preferred_build_system) + " didn't work?");
    for (Supported system : build_systems) {
      if (is_build_system_installed(system) == false) {
        continue;
      }
      if (system == Supported::UNSET)
        continue;
      if (try_build(pkg, get_build_command(system, path)))
        return system;
    }
    return Supported::UNSET;
  }

  bool is_build_system_installed(Supported system) {
    switch (system) {
    case Supported::CMAKE:
      return is_tool_available("cmake");
      ;
    case Supported::AUTOCRAFT:
      // TODO: check a compiler here later
      return false;
    case Supported::MAKE:
      return is_tool_available("make");
      ;
    case Supported::MESON:
      if (is_tool_available("make") && is_tool_available("meson"))
        return true;
    case Supported::ZIG:
      is_tool_available("zig");
      return true;
    case Supported::SCONS:
      is_tool_available("scons");
      return true;
    case Supported::AUTOTOOLS:
      // TODO: figure out what to check for gnu autotools
      return true;
    case Supported::PREMAKE:
      if (is_tool_available("premake5") && is_tool_available("meson"))
        return true;
    case Supported::CUSTOM:
      return true;
    case Supported::UNSET:
      return false;
    default:
      return false;
    }
  }

  void add_headers_to_project(const Package &pkg) {
    for (auto &h : pkg.headers_path)
      include_headers.emplace_back(h);
    for (auto &h : pkg.additional_headders_path)
      include_headers.emplace_back(to_path({libs_dir, pkg.name, h}));
  }

  // The main build function
  void build_package(const Package &pkg_og) {
    // Keeping a copy of the original user config to compare with and make
    // cache out of
    const Package &pkg_ref = pkg_og;
    Package pkg = pkg_og; // a copy to work with

    fs::path path = libs_dir / pkg.name;

    if (!pkg.enabled) {
      log(LogLevel::INFO, pkg.name + " is disabled. Skipping build & link.");
      return;
    }

    detect_artifact_paths(pkg, path);
    detect_artifact_names(pkg, path);
    detect_headers(pkg, path);

    // Skip building if it's header-only or precompiled
    if (pkg.is_header_only) {
      log(LogLevel::INFO,
          pkg.name + " is header-only. Only including headers.");
      add_headers_to_project(pkg);
      return;
    }

    if (pkg.is_precompiled_binary) {
      log(LogLevel::INFO, pkg.name + " is precompiled. Skipping build.");
      add_headers_to_project(pkg);
      linked_libraries.emplace_back(pkg);
      return;
    }

    if (pkg.preferred_build_system == Supported::UNSET) {
      pkg.preferred_build_system = detect_build_system(pkg, path);
    }

    if (pkg.preferred_build_system == Supported::UNSET) {
      log(LogLevel::INFO, "No build system found, for: " + pkg.name +
                              " treating this Package "
                              "as an include header only");
      add_headers_to_project(pkg);
      return;
    }

    // Check cache with the pkg un changed from the user, but run the
    // autodection if it's cached
    if (is_package_cached(pkg_ref)) {
      log(LogLevel::INFO, pkg.name + " is cached. Skipping build.");
      add_headers_to_project(pkg);
      linked_libraries.emplace_back(pkg);
      return;
    }

    // Handle custom build commands
    if (!pkg.custom_build_cmd.empty()) {
      if (!try_build(pkg,
                     "cd " + path.string() + " && " + pkg.custom_build_cmd)) {
        log(LogLevel::ERROR,
            "Custom build command failed: " + pkg.custom_build_cmd);
        return;
      }
      add_headers_to_project(pkg);
      linked_libraries.emplace_back(pkg);
      return;
    }

    // Try building with detected build systems
    Supported chosen_build_system = try_build_systems(pkg, path);
    switch (chosen_build_system) {
    case Supported::UNSET:
      log(LogLevel::ERROR, "Failed to build " + pkg.name);
      return;
      break;
    case Supported::ZIG:
      log(LogLevel::INFO, "Used Zig, hard coding to zig-out");
      pkg.headers_path = {to_path({path, "zig-out", "include"})};
      pkg.artifact_dir = {to_path({path, "zig-out", "lib"})};
      break;
    default:
      break;
    }
    add_headers_to_project(pkg);
    linked_libraries.emplace_back(pkg);
  }

  int run_binary(const std::string &binary_name) {
#ifdef _WIN32
#define EXEC_PREFIX ""
#define EXEC_SUFFIX ".exe"
#else
#define EXEC_PREFIX "./"
#define EXEC_SUFFIX ""
#endif

    std::string command = EXEC_PREFIX + binary_name + EXEC_SUFFIX;
    log(LogLevel::INFO, "Running: " + command);
    return std::system(command.c_str());
  }

  uint64_t murmur3_64(const uint8_t *key, size_t len,
                      uint64_t seed = 0xc6a4a7935bd1e995ULL) {
    uint64_t h = seed ^ (len * 0xc6a4a7935bd1e995ULL);
    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len / 8);

    while (data != end) {
      uint64_t k = *data++;
      k *= 0xc6a4a7935bd1e995ULL;
      k ^= k >> 47;
      k *= 0xc6a4a7935bd1e995ULL;
      h ^= k;
      h *= 0xc6a4a7935bd1e995ULL;
    }

    const uint8_t *data2 = (const uint8_t *)data;
    uint64_t k = 0;
    switch (len & 7) {
    case 7:
      k ^= (uint64_t)data2[6] << 48;
    case 6:
      k ^= (uint64_t)data2[5] << 40;
    case 5:
      k ^= (uint64_t)data2[4] << 32;
    case 4:
      k ^= (uint64_t)data2[3] << 24;
    case 3:
      k ^= (uint64_t)data2[2] << 16;
    case 2:
      k ^= (uint64_t)data2[1] << 8;
    case 1:
      k ^= (uint64_t)data2[0];
      k *= 0xc6a4a7935bd1e995ULL;
      k ^= k >> 47;
      k *= 0xc6a4a7935bd1e995ULL;
      h ^= k;
    }

    h ^= h >> 47;
    h *= 0xc6a4a7935bd1e995ULL;
    h ^= h >> 47;
    return h;
  }

  uint64_t hash_package(const std::string &data) {
    return murmur3_64(reinterpret_cast<const uint8_t *>(data.c_str()),
                      data.size());
  }

  void cache_package(const Package &pkg) {
    fs::path _built_dir = to_path({libs_dir, ".built"});
    fs::path _built_pkg_dir = _built_dir / pkg.name;

    if (!fs::exists(_built_dir)) {
      log(LogLevel::DEBUG,
          "creating the cache directory in: " + _built_dir.string());
      fs::create_directories(_built_dir);
    }
    if (!fs::exists(_built_pkg_dir)) {
      log(LogLevel::DEBUG, "creating the cache directory for:" + pkg.name +
                               " in: " + _built_dir.string());
      fs::create_directories(_built_pkg_dir);
    }

    for (const auto &entry : fs::directory_iterator(_built_pkg_dir)) {
      fs::remove_all(entry.path());
    }

    fs::path marker_path = _built_pkg_dir / pkg.commit;
    std::ofstream marker(marker_path);
    if (marker.fail()) {
      log(LogLevel::ERROR,
          "Failed to create marker file: " + marker_path.string());
      return;
    }
    uint64_t hash_value = hash_package(serialize_package(pkg));
    marker << std::to_string(hash_value) << std::endl;
    marker.close();
  }

  // I mean now the build function take a compiler command the fallback is
  // redundant? or can we just off this function in the api?
  std::string choose_compiler() {
    if (project_compiler.empty()) {
      if (is_tool_available("g++")) {
        return "g++";
      } else if (is_tool_available("clang++")) {
        return "clang++";
      }
#ifdef _WIN32
      if (is_tool_available("cl")) {
        return "cl";
      }
#endif
      log(LogLevel::ERROR,
          "coulnd't find a compiler please specify one, Exiting");
    }
    return "";
  }

  int generate_compile_json(std::string &compile_command) {
    // Get the absolute path of the current directory (project root)
    std::string project_dir = fs::current_path().string();

    // Define your compile command and the source file
    std::string source_file; // Example source file
    for (auto &f : sources) {
      source_file += f + " ";
    }

    // Construct the compile_commands.json content manually
    std::string json_content = R"([
    {
        "directory": ")" + project_dir +
                               R"(",
        "command": ")" + compile_command +
                               R"(",
        "file": ")" + source_file +
                               R"("
    } ])";
    // Write the JSON content to compile_commands.json
    std::ofstream file("compile_commands.json");
    if (file.is_open()) {
      file << json_content;
      file.close();
      log(LogLevel::INFO, "Generated compile_commands.json successfully!");
    } else {
      log(LogLevel::ERROR, "Failed to open compile_commands.json for writing.");
      file.close();
      return 1;
    }
    return 0;
  }
};

#ifdef _WIN32
#include <windows.h>
void enable_windows_ansi() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif
#endif // SCOPE_EXIT_HPP
