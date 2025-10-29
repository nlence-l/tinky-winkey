#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

int main() 
{
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string filePath = std::string(tempPath) + "keylogger.log";
    
    // Create initial log entry
    std::ofstream file(filePath, std::ios::app);
    if (file.is_open()) 
    {
        file << "=== Keylogger Started ===\n";
        file << "Timestamp: " << __DATE__ << " " << __TIME__ << "\n";
        file << "Process ID: " << GetCurrentProcessId() << "\n";
        file << "Location: " << filePath << "\n";
        file << "========================\n\n";
        file.close();
        printf("Keylogger started successfully!\n");
        printf("Log file: %s\n", filePath.c_str());
    }
    else 
    {
        printf("ERROR: Failed to create log file at: %s\n", filePath.c_str());
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    
    // Keep running and log periodically
    int counter = 0;
    printf("Keylogger running in background...\n");
    
    while (true) 
    {
        Sleep(10000); // Sleep 10 seconds
        
        // Append to log file
        std::ofstream logFile(filePath, std::ios::app);
        if (logFile.is_open()) 
        {
            logFile << "Keylogger active - Cycle: " << ++counter << "\n";
            logFile.close();
        }
    }
    
    return 0;
}