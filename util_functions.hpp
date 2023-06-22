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
#include <sys/stat.h>

// Set standard (std) namespace.
using namespace std;

// Declare functions.
void xlog(const string& log_type, const string& message);
void write(const string& path, const string& contents);
string print_date(const string& date_format);
string exec_shell(const string& cmd, bool need_output);
template <typename T, typename U>U convert(T value);
pid_t get_pid(const string& process_name);
vector<pid_t> get_pids(const string &process_name);
pid_t get_proc_tid(const string &process_name, const string &thread_name);
vector<pid_t> get_proc_tids(const string &process_name, const string &thread_name);
void renice_process(const string& process_name, int new_priority);
void kill_process(const string& process_name);
void kill_processes(const string& process_name);
void kill_thread(const string& process_name, const string& thread_name);
void kill_threads(const string& process_name, const string& thread_name);
void change_process_cpu_affinity(const string& process_name, int start, int end);
void change_thread_cpu_affinity(const string& process_name, const string& thread_name, int start, int end);
void change_process_scheduler(const string& process_name, const string& sched_policy, int priority);
void change_thread_scheduler(const string& process_name, const string& thread_name, const int& sched_policy, int priority);
string get_home_pkg_name();
string get_ime_pkg_name();
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
    cerr << "[Error] " << message << endl;
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

pid_t get_pid(const string &process_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "ps -Ao pid,args | grep -i -E '" + process_name + "' | awk '{print $1}' | head -n 1";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);  
  
  // Use convert() function to convert string to pid_t & return.
  return convert<string, pid_t>(output);
}

vector<pid_t> get_pids(const string &process_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "ps -Ao pid,args | grep -i -E '" + process_name + "' | awk '{print $1}'";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);  

  // Split the output by line, convert string to pid_t and store it in a vector.
  vector<pid_t> list;
  stringstream ss(output);
  string line;
  while (getline(ss, line, '\n')) { 
    list.push_back(convert<string, pid_t>(line)); 
  }
  return list;    
}

pid_t get_proc_tid(const string &process_name, const string &thread_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "pid=$(pidof -s " + process_name + "); for tid in $(ls /proc/$pid/task/); do thread_name=$(cat /proc/$pid/task/$tid/comm); [[ \"$(echo $thread_name | grep -i -E " + thread_name + ")\" != \"\" ]] && echo $tid && break; done";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);  
  
  // Use convert() function to convert string to pid_t & return.
  return convert<string, pid_t>(output);
}

vector<pid_t> get_proc_tids(const string &process_name, const string &thread_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "pid=$(pidof -s " + process_name + "); for tid in $(ls /proc/$pid/task/); do thread_name=$(cat /proc/$pid/task/$tid/comm); [[ \"$(echo $thread_name | grep -i -E " + thread_name + ")\" != \"\" ]] && list=\"$list\n$tid\" && break; done; echo $list";
  
  // Use exec_shell() function to execute the shell command.
  output = exec_shell(cmd, true);    
  
  // Split the output by line, convert string to pid_t and store it in a vector.
  vector<pid_t> list;
  stringstream ss(output);
  string line;
  while (getline(ss, line, '\n')) { 
    list.push_back(convert<string, pid_t>(line)); 
  }
  return list;    
}

void renice_process(const string& process_name, int new_priority) {
  // Get the PID of the given process name using get_pid() function.
  pid_t pid = get_pid(process_name);

  // Set the priority of the process.
  setpriority(PRIO_PROCESS, pid, new_priority);
}

void renice_thread(const string& process_name, const string& thread_name, int new_priority) {
  // Get the TID of the given thread name using get_proc_tid() function.
  pid_t tid = get_proc_tid(process_name, thread_name);

  // Set the priority of the thread.
  setpriority(PRIO_PROCESS, tid, new_priority);
}

// Process killer function.
void kill_process(const string& process_name) {
  // Get the PID of the given process name using get_pid() function.
  pid_t pid = get_pid(process_name);
    
  // Send the SIGKILL signal to the process with the specified PID.
  kill(pid, SIGKILL);
}

// Kill processes with same name.
void kill_processes(const string& process_name) {
  // Get list of pids & store as vector.
  vector<pid_t> pids = get_pids(process_name);

  // Iterate through the list of pids.
  for (pid_t pid : pids) {
    // Send the SIGKILL signal to the process using the given PID.
    kill(pid, SIGKILL);  
  }
}

// Thread killer function.
void kill_thread(const string& process_name, const string& thread_name) {
  // Get the TID of the given thread name using get_proc_tid() function.
  pid_t tid = get_proc_tid(process_name, thread_name);
    
  // Send the SIGKILL signal to the process with the specified TID.
  kill(tid, SIGKILL);
}

// Kill threads with same name.
void kill_threads(const string& process_name, const string& thread_name) {
  // Get list of tids & store as vector.
  vector<pid_t> tids = get_proc_tids(process_name, thread_name);

  // Iterate through the list of tids.
  for (pid_t tid : tids) {
    // Send the SIGKILL signal to the process using the given TID.
    kill(tid, SIGKILL);  
  }
}

// Change CPU affinity of a process.
void change_process_cpu_affinity(const string& process_name, int start, int end) {
  // Get the PID of the given process name using get_pid() function.
  pid_t pid = get_pid(process_name);

  // Initialize the mask to all zeros.
  cpu_set_t mask;
  CPU_ZERO(&mask);  

  // Set the specified range of CPUs in the mask.
  for (int i = start; i <= end; i++) {
    CPU_SET(i, &mask);
  }
  
  // Set the CPU affinity to the process from the mask.
  int result = sched_setaffinity(pid, sizeof(mask), &mask);
  
  // Check the return value to see if the call was successful.  
  if (result == -1) {
    xlog("error", "Failure setting CPU affinity: " + process_name);
  }
}

// Change CPU affinity of a thread.
void change_thread_cpu_affinity(const string& process_name, const string& thread_name, int start, int end) {
  // Get the TID of the given thread name using get_proc_tid() function.
  pid_t tid = get_proc_tid(process_name, thread_name);

  // Initialize the mask to all zeros.
  cpu_set_t mask;
  CPU_ZERO(&mask);  

  // Set the specified range of CPUs in the mask.
  for (int i = start; i <= end; i++) {
    CPU_SET(i, &mask);
  }
  
  // Set the CPU affinity to the thread from the mask.
  int result = sched_setaffinity(tid, sizeof(mask), &mask);
  
  // Check the return value to see if the call was successful.  
  if (result == -1) {
    xlog("error", "Failure setting CPU affinity: " + process_name + ":" + thread_name);
  }
}

// Change the CPU scheduling policy of a process.
void change_process_scheduler(const string& process_name, const int& sched_policy, int priority) {
  // Get the PID of the given process name using get_pid() function.
  pid_t pid = get_pid(process_name);

  // Set the scheduling policy for the current process.
  sched_param param;
  param.sched_priority = priority;
  int result = sched_setscheduler(pid, sched_policy, &param);

  // Check the return value to see if the call was successful.
  if (result == -1) {
    xlog("error", "Failure changing scheduler: " + process_name);
  }  
}

// Change the CPU scheduling policy of a thread.
void change_thread_scheduler(const string& process_name, const string& thread_name, const int& sched_policy, int priority) {
  // Get the TID of the given thread name using get_proc_tid() function.
  pid_t tid = get_proc_tid(process_name, thread_name);

  // Set the scheduling policy for the thread.
  sched_param param;
  param.sched_priority = priority;
  int result = sched_setscheduler(tid, sched_policy, &param);

  // Check the return value to see if the call was successful.
  if (result == -1) {
    xlog("error", "Failure changing scheduler: " + process_name + ":" + thread_name);
  }  
}

// Get home launcher's package name.
string get_home_pkg_name() {
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
string get_ime_pkg_name() {
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
  struct stat info;
  int ret = stat(path.c_str(), &info);
  return ret == 0;
}

// Get list of paths from wildcard path. (e.g /foo/*.txt)
vector<string> get_paths_from_wp(const string& wildcard_path) {
  // Execute a shell command to retrieve the output of the wildcard expansion.
  string output = exec_shell("for i in " + wildcard_path + "; do echo \"$i\"; done", true);

  // Split the output by line and store it in a vector.
  vector<string> list;
  stringstream ss(output);
  string line;
  while (getline(ss, line, '\n')) { 
    list.push_back(line); 
  }
  return list;    
}

// remove() with support to delete wildcard path.
void remove_path(const string& path) {
  // If the path contains an asterisk, then use the get_paths_from_wp() function to get a list of paths.
  if (path.find('*') != string::npos) { 
    vector<string> paths;
    paths = get_paths_from_wp(path);
    for (const std::string& path : paths) {
      remove(path.c_str());
    }
  } else {
    // If path does not contain an asterisk, then continue with normal method.
    remove(path.c_str());
  }
}

// Check if app exists using `PackageManager`.
bool is_app_exists(const string& pkg_name) {
  // Build shell command to execute.
  string cmd = "pm path " + pkg_name + " &>/dev/null && echo true || echo false";
  
  // Return true under condition else false.
  if (exec_shell(cmd, true) == "true") {
    return true;
  } else { 
    return false;
  }
}