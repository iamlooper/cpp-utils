#include <iostream>
#include <sys/resource.h>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <sched.h>
#include <csignal>
#include <cstring>
#include <vector> 
#include <fstream>
#include <cerrno>
#include <dirent.h>
#include <libgen.h>
#include <regex>
#include <unordered_map>
#include <filesystem>

// Set standard (std) namespace.
using namespace std;

// Declare functions.
void xlog(const string& log_type, const string& message);
void write(const string& path, const string& contents);
string print_date(const string& date_format);
string exec_shell(const string& cmd, bool need_output);
template <typename T, typename U>U convert(T value);
string get_pid(const string& process_name);
vector<string> get_pid_list(const string &process_name);
void renice_process(const string& process_name, int new_priority);
void kill_process(const string& process_name);
void set_cpu_affinity(const string& process_name, int start, int end);
void change_scheduler(const string& process_name, const string& sched_policy, int priority);
string get_home_pkgname();
string get_ime_pkgname();
void mlock_item(string mlock_type, string object);
string escape_special_characters(const string& text);
string grep_prop(const string& property, const string& filename);
bool is_net_connection();
void web_fetch(const string& link, const string& file_path);
bool is_path_exists(const string& path);
vector<string> get_paths_from_wp(const string& wildcard_path);
void remove_path(const string& path);

// Logger function.
void xlog(const string& log_type, const string& message) {
  // Write to log file depending on log type.
  if (log_type == "info") {
    cout << "[Info] " << message << endl;
  } else if (log_type == "error") {
    cout << "[Error] " << message << endl;
  } else if (log_type == "date") {
    cout << "[" + print_date("half") + "] " << message << endl;
  } else {
    // Write a generic message.
    cout << message << endl;
  }
}

// Write contents to a file.
void write(const string& path, const string& contents) {
  // Open the file.
  ofstream file(path);
  // If the file was successfully opened.
  if (file.is_open()) {
    // Write the contents to the file.
    file << contents;
    // Close the file.
    file.close();
  } else {
    // Print an error message if the file cannot be opened.
    xlog("error", "Failure opening file: " + path);
    // Close the file.
    file.close();    
  }
}

// Print the current date to the standard output stream and return it as a string.
string print_date(const string& date_format) {
  // Get the current time using std::system_clock.
  time_t current_time = time(nullptr);

  // Format the time using std::put_time.
  stringstream date;
  if (date_format == "full") {
    date << put_time(localtime(&current_time), "%Y-%m-%d %H:%M:%S");
  } else if (date_format == "half") {
    date << put_time(localtime(&current_time), "%H:%M:%S");  
  } else { 
    return "";
  } 
  
  // Return as string.  
  return date.str();
}

// Shell command execution using popen().
string exec_shell(const string& cmd, bool need_output) {
  // Create a stringstream to store the output.
  stringstream output;
  // Create a vector of characters to store the buffer.
  vector<char> buffer(BUFSIZ);

  // Open a pipe to the specified shell command and read/write to it using a FILE object.
  FILE* pipe = popen(cmd.c_str(), "r");
  if (pipe == nullptr) {
    // Error opening the pipe.
    return "";
  }  

  // Check if the output is needed.
  if (need_output) {
    // Read the output from the pipe in chunks.
    while (fgets(buffer.data(), buffer.size(), pipe)) {
      // Get the length of the output in the buffer.
      size_t length = strlen(buffer.data());
      // Check if the buffer size is enough.
      if (length == buffer.size() - 1 && buffer[length - 1] != '\n') {
        // If the buffer size is not enough, double it and read again.
        buffer.resize(buffer.size() * 2);
        continue;
      }
      // Add the buffer to the sringstream.
      output << buffer.data();
    }
  }

  // Close the pipe.
  int error = pclose(pipe);
  if (error != 0) {
    // Error closing the pipe.
    return "";
  }  
  
  // Check if the output is needed.
  if (need_output) { 
    // Store the result in a string.
    string result = output.str();
    // Check if the result has a newline character at the end.
    if (result.length() > 0 && result[result.length() - 1] == '\n') {
      // If it does, remove the newline character.
      result.erase(result.length() - 1);
    }  
    // Return the result.
    return result;
  } else { 
    // Return an empty string if output is not needed.
    return ""; 
  }
}

// Data type converter.
template <typename T, typename U>U convert(T value) {
  // Initialize stringstream variable. 
  stringstream stream;

  // Initialize result data type that we need.
  U result;

  // Feed to-be converted data type to stringstream.
  stream << value;

  // Convert data type to result data type using stringstream.
  stream >> result;

  // Return result.
  return result;
}

string get_pid(const string &process_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "ps -Ao pid,args | grep -i -E '" + process_name + "' | awk '{print $1}' | head -n 1";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);  

  // Return the output.
  return output;
}

vector<string> get_pid_list(const string &process_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "ps -Ao pid,args | grep -i -E '" + process_name + "' | awk '{print $1}'";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);  

  // Split output by line and return as vector.
  vector<string> pid_list;
  stringstream ss(output);
  string pid;
  while (getline(ss, pid, '\n')) { 
    pid_list.push_back(pid); 
  }
  return pid_list;
}

void renice_process(const string& process_name, int new_priority) {
  // Get the process ID of the current process using get_pid() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pid(process_name));

  // Set the priority of the process.
  setpriority(PRIO_PROCESS, pid, new_priority);
}

// Process killer function.
void kill_process(const string& process_name) {
  // Get the process ID of the current process using get_pid() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pid(process_name));
    
  // Send the SIGKILL signal to the process with the specified PID.
  kill(pid, SIGKILL);
}

// Change CPU affinity of a process.
void set_cpu_affinity(const string& process_name, int start, int end) {
  // Get the process ID of the current process using get_pid() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pid(process_name));

  // Initialize the mask to all zeros.
  cpu_set_t mask;
  CPU_ZERO(&mask);  

  // Set the specified range of CPUs in the mask.
  for (int i = start; i <= end; i++) {
    CPU_SET(i, &mask);
  }

  int result = sched_setaffinity(pid, sizeof(mask), &mask);
  if (result == -1) {
    xlog("error", "Failure setting CPU affinity: " + process_name);
  }
}

// Change CPU scheduling of a process.
void change_scheduler(const string& process_name, const int& sched_policy, int priority) {
  // Get the process ID of the current process using get_pid() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pid(process_name));

  // Set the scheduling policy for the current process to SCHED_FIFO.
  sched_param param;
  param.sched_priority = priority;
  int result = sched_setscheduler(pid, sched_policy, &param);

  // Check the return value to see if the call was successful.
  if (result == -1) {
    xlog("error", "Failure changing scheduler: " + process_name);
  }  
}

// Get home launcher's package name.
string get_home_pkgname() {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "pm resolve-activity -a android.intent.action.MAIN -c android.intent.category.HOME | grep packageName | head -n 1 | cut -d= -f2";
  
  // Use exec_shell() function to execute shell command.
  output = exec_shell(cmd, true);  

  // Return the output.
  return output;  
}

// Get IME's package name.
string get_ime_pkgname() {
  // Initialize variable to read output. 
  string output;

  // Build command to execute.
  string cmd = "ime list | grep packageName | head -n 1 | cut -d= -f2";
  
  // Use exec_shell() function to execute shell command.
  output = exec_shell(cmd, true);  

  // Return the output.
  return output;  
}

// Preload or lock a item in memory.
void preload_item(string item_type, string item) {
  // Initialize variable to build shell command.
  string cmd;

  // Preload depending on item type.
  if (item_type == "obj") {
    cmd = "/data/local/tmp/vmtouch -dl " + item;
  } else if (item_type == "app") {
    cmd = "/data/local/tmp/vmtouch -dl \"$(pm path " + item + " | cut -d: -f2 | xargs dirname)\"";
  } else if (item_type == "dex") {
    cmd = "apk_path=\"$(pm path " + item + " | cut -d: -f2 | xargs dirname)\" && /data/local/tmp/vmtouch -dl \"$apk_path/oat\"";
  } else {
    return;
  }

  // Use exec_shell() function to execute shell command.
  exec_shell(cmd, false);
}

// Escape special characters.
string escape_special_characters(const string& text) {
  string escaped_text = text;
  
  escaped_text = regex_replace(escaped_text, regex("\""), "\\\""); // Double quote
  escaped_text = regex_replace(escaped_text, regex("\'"), "\\'"); // Single quote
  escaped_text = regex_replace(escaped_text, regex("\\"), "\\\\"); // Backslash
  
  // Return as string.
  return escaped_text;
}

// Property fetcher.
string grep_prop(const string& property, const string& filename) {
  // Create a map to store the key-value pairs from the file.
  unordered_map<string, string> properties;

  // Open the file for reading.
  ifstream file(filename);

  // Check if the file was successfully opened.
  if (!file) {
    // Print an error message and return an empty string.
    xlog("error", "Unable to open file " + filename + " for reading.");
    return "";
  }

  // Read the key-value pairs from the file and store them in the map.
  string line;
  while (getline(file, line)) {
    // Find the first "=" character in the line.
    auto pos = line.find('=');
    if (pos == string::npos) {
      continue; // Skip this line if it does not contain an "=" character.
    }

    // Split the line at the "=" character and store the key and value in the map.
    auto key = line.substr(0, pos);
    auto value = line.substr(pos + 1);

    // Trim any whitespace from the beginning and end of the value.
    value = regex_replace(value, regex("^\\s+|\\s+$"), "");

    // Check if the value starts with a quote character.
    if (value[0] == '"' || value[0] == '\'') {
      // If the value starts with a quote character, find the matching quote character.
      auto endPos = value.find_first_of(value[0], 1);
      if (endPos != string::npos) {
        // If a matching quote character was found, remove the quotes from the value.
        value = value.substr(1, endPos - 1);
      }
    }
    properties[key] = value;
  }

  // Return the value of the specified property from the map.
  return properties[property];
}

// Returns true if a network connection is established, false otherwise.
bool is_net_connection() {
  // Build shell command to execute.
  string cmd = "ping -c 1 linkedin.com &>/dev/null && echo true || echo false";
  
  // Return true under condition else false.
  if (exec_shell(cmd, true) == "true") {
    return true;
  } else { 
    return false;
  }
}

// Web fetch tool.
void web_fetch(const string& link, const string& file_path) {
  // Build shell command to execute.
  string cmd = "/data/adb/magisk/busybox wget " + link + " -qO " + file_path;
  
  // Use exec_shell() function to execute shell command.
  exec_shell(cmd, false);
}

// Check if path exists.
bool is_path_exists(const string& path) {
  return filesystem::exists(path);
}

// Get list of paths from wildcard path. (e.g /foo/*.txt)
vector<string> get_paths_from_wp(const string& wildcard_path) {
  // Replace wildcard characters with their regex equivalents.
  string regex_str = regex_replace(wildcard_path, regex(R"(\.)"), R"(\.)");
  regex_str = regex_replace(regex_str, regex(R"(\*)"), R"(.*)");
  regex_str = regex_replace(regex_str, regex(R"(\?)"), R"(.)");
  regex path_regex(regex_str);

  // Initialize a vector to store matched paths.
  vector<string> matched_paths;

  // Get the base path from the wildcard path.
  filesystem::path base_path = filesystem::path(wildcard_path).parent_path();
  
  // Check if the base path exists.
  if (!is_path_exists(base_path)) {
    xlog("error", "Base path does not exist: " + base_path);
    return matched_paths;
  }  

  // Iterate through the directory entries.
  for (const auto& entry : filesystem::directory_iterator(base_path)) {
    // Check if the current entry's path matches the regex pattern.
    if (regex_match(entry.path().string(), path_regex)) {
      matched_paths.push_back(entry.path().string());
    }
  }

  return matched_paths;
}

// remove() with support to delete wildcard path.
void remove_path(const string& path) {
  // If the path contains an asterisk, then use the get_paths_from_wp() function to get a list of paths.
  if (path.find('*') != string::npos) {
    vector<string> paths = get_paths_from_wp(path);
    for (const std::string& path : paths) {
      remove(path.c_str());
    }
  } else {
    // If path does not contain an asterisk, then continue with normal method.
    remove(path.c_str());
  }
}