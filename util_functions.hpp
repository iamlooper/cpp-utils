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

// Set standard (std) namespace.
using namespace std;

// Declare functions.
void xlog(const string& log_type, const string& message);
void write(const string& path, const string& contents);
string print_date(const string& date_format);
string exec_shell(const string& cmd, bool need_output);
template <typename T, typename U>U convert(T value);
string get_pidof(const string &process_name);
void renice_process(const string& process_name, int new_priority);
void force_kill(const string& process_name);
void set_cpu_affinity(const string& process_name, int start, int end);
void change_scheduler(const string& process_name, const string& sched_policy, int priority);
string get_home_pkgname();
string get_ime_pkgname();
void mlock_item(string mlock_type, string object);
string escape_special_characters(const string& text);
string grep_prop(const string& property, const string& filename);
bool is_net_connection();
void web_fetch(const string& link, const string& file_path);

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
  ofstream ofs(path);
  // If the file was successfully opened.
  if (ofs.is_open()) {
    // Write the contents to the file.
    ofs << contents;
    // Close the file.
    ofs.close();
  } else {
    // Print an error message if the file cannot be opened.
    xlog("error", "Failure opening file: " + path);
    // Close the file.
    ofs.close();    
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

string get_pidof(const string &process_name) {
  // Initialize variable to read output.
  string output;

  // Build command to execute.
  string cmd = "ps -Ao pid,args | grep -i -E '" + process_name + "' | awk '{print $1}' | head -n 1";
  
  // Use exec_shell() function to execute shell commands.
  output = exec_shell(cmd, true);  

  // Return the output.
  return output;
}

void renice_process(const string& process_name, int new_priority) {
  // Get the process ID of the current process using get_pidof() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pidof(process_name));

  // Set the priority of the process.
  setpriority(PRIO_PROCESS, pid, new_priority);
}

// Process killer function.
void force_kill(const string& process_name) {
  // Get the process ID of the current process using get_pidof() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pidof(process_name));
    
  // Send the SIGKILL signal to the process with the specified PID.
  kill(pid, SIGKILL);
}

// Change CPU affinity of a process.
void set_cpu_affinity(const string& process_name, int start, int end) {
  // Get the process ID of the current process using get_pidof() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pidof(process_name));

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
  // Get the process ID of the current process using get_pidof() function.
  // Use convert() function to convert string to pid_t.
  pid_t pid = convert<string, pid_t>(get_pidof(process_name));

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