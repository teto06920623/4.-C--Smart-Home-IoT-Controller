#include <iostream>
#include <string>
#include <iomanip>
#include <winsock2.h>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// --- 1. Abstract Device (The Receiver) ---
class Device
{
protected:
    int deviceID;
    string deviceName;
    bool isOn;
    double energyConsumption;

public:
    Device(int id, string name) : deviceID(id), deviceName(name), isOn(false), energyConsumption(0.0) {}
    virtual ~Device() {}
    virtual void toggleStatus() = 0;
    virtual void calculateEnergy() = 0;

    string generateNetworkResponse() const
    {
        return "STATUS:" + string(isOn ? "ON" : "OFF") + ",ENERGY:" + to_string(energyConsumption);
    }
    string getName() const { return deviceName; }
    bool getStatus() const { return isOn; }
};

class SmartLight : public Device
{
public:
    SmartLight(int id, string name) : Device(id, name) {}
    void toggleStatus() override
    {
        isOn = !isOn;
    }
    void calculateEnergy() override
    {
        energyConsumption = isOn ? 5.0 : 0.0;
    }
};

// --- 2. The Command Pattern Architecture ---
class Command
{
public:
    virtual void execute() = 0;
    virtual ~Command() {}
};

// Concrete Command: Toggling a Device
class ToggleCommand : public Command
{
private:
    Device *targetDevice;

public:
    ToggleCommand(Device *device) : targetDevice(device) {}

    void execute() override
    {
        if (targetDevice != nullptr)
        {
            targetDevice->toggleStatus();
            targetDevice->calculateEnergy();
        }
    }
};

// Concrete Command: Graceful Shutdown
class ExitCommand : public Command
{
private:
    bool &serverRunning; // Reference to the state machine flag
public:
    ExitCommand(bool &flag) : serverRunning(flag) {}

    void execute() override
    {
        serverRunning = false;
        cout << "\n[System] EXIT command received. Initiating graceful shutdown..." << endl;
    }
};

// --- 3. The System Controller (Home) ---
class Home
{
private:
    Device *devices[100];
    int deviceCount;
    int failedAttempts;
    const string ADMIN_PASS = "admin123";

public:
    Home() : deviceCount(0), failedAttempts(0)
    {
        for (int i = 0; i < 100; i++)
            devices[i] = nullptr;
    }

    virtual ~Home()
    {
        for (int i = 0; i < deviceCount; i++)
            delete devices[i];
    }

    void addDevice(Device *newDevice)
    {
        if (deviceCount < 100)
        {
            devices[deviceCount] = newDevice;
            deviceCount++;
        }
    }

    bool login()
    {
        string inputPass;
        cout << "\n=== ADMIN LOGIN ===" << endl;
        while (failedAttempts < 3)
        { // [cite: 307]
            cout << "Enter Password: ";
            cin >> inputPass;
            if (inputPass == ADMIN_PASS)
            {
                cout << "[Auth] Login Successful." << endl;
                return true;
            }
            failedAttempts++;
            cout << "[Auth] Incorrect password. Attempts remaining: " << (3 - failedAttempts) << endl;
        }
        cout << "[SECURITY ALERT] Maximum attempts reached. Terminating..." << endl;
        exit(1);
    }

    void render2DGrid() const
    { // [cite: 317]
        cout << "\n================= SMART HOME 2D GRID =================\n";
        const int COLS = 4;
        for (int i = 0; i < deviceCount; i++)
        {
            string statusStr = devices[i]->getStatus() ? " [ON] " : " [OFF]";
            cout << "| " << setw(15) << left << devices[i]->getName() << statusStr << " | ";
            if ((i + 1) % COLS == 0)
            {
                cout << "\n";
                for (int j = 0; j < COLS; j++)
                    cout << "-----------------------------";
                cout << "\n";
            }
        }
        cout << "\n======================================================\n";
    }

    Device *getDevice(int index)
    {
        if (index >= 0 && index < deviceCount)
            return devices[index];
        return nullptr;
    }
    // دالة جديدة لتوليد الشبكة كنص لإرسالها عبر الشبكة
    string generate2DGridString() const
    {
        stringstream ss;
        ss << "\n================= SMART HOME 2D GRID =================\n";
        const int COLS = 4;
        for (int i = 0; i < deviceCount; i++)
        {
            string statusStr = devices[i]->getStatus() ? " [ON] " : " [OFF]";
            ss << "| " << setw(15) << left << devices[i]->getName() << statusStr << " | ";
            if ((i + 1) % COLS == 0 || i == deviceCount - 1)
            {
                ss << "\n";
                for (int j = 0; j < COLS; j++)
                    ss << "-----------------------------";
                ss << "\n";
            }
        }
        ss << "======================================================\n";
        return ss.str();
    }
};

// --- 4. Main Execution (The Invoker & Network Stack) ---
int main()
{
    Home cyberCoreHome;
    cyberCoreHome.login();

    cyberCoreHome.addDevice(new SmartLight(101, "Living Room"));
    cyberCoreHome.addDevice(new SmartLight(102, "Bedroom"));
    cyberCoreHome.addDevice(new SmartLight(103, "Kitchen"));
    cyberCoreHome.addDevice(new SmartLight(104, "Garage"));

    cyberCoreHome.render2DGrid();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 3);

    cout << "\n[System] TCP Server is ONLINE. Listening on PORT 8080..." << endl;

    bool isRunning = true; // State flag for Persistence

    while (isRunning)
    {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
            continue;

        char buffer[1024] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0)
        {
            string payload(buffer);
            Command *cmd = nullptr; // Polymorphic Command Pointer

            // Parsing the payload to dynamically create the correct Command
            if (payload.find("EXIT") != string::npos)
            {
                cmd = new ExitCommand(isRunning);
            }
            else if (payload.find("TOGGLE_LIGHT") != string::npos)
            {
                cmd = new ToggleCommand(cyberCoreHome.getDevice(0));
            }

            // Execute the command if it's valid
            // Execute the command if it's valid
            if (cmd != nullptr)
            {
                cmd->execute(); // Dynamic Dispatch happens here!

                // If it was a Toggle, update the UI and send response
                if (isRunning)
                {
                    cyberCoreHome.render2DGrid(); // لطباعتها في السيرفر أيضاً

                    // توليد الرد المدمج
                    string baseResponse = cyberCoreHome.getDevice(0)->generateNetworkResponse();
                    string gridResponse = cyberCoreHome.generate2DGridString();
                    string fullPayload = baseResponse + "\n_GRID_START_\n" + gridResponse;

                    // إرسال الحزمة الكاملة
                    send(clientSocket, fullPayload.c_str(), fullPayload.length(), 0);
                }
                delete cmd;
            }
        }
        closesocket(clientSocket);
    }

    // Graceful Shutdown: Releasing Network Resources
    cout << "[System] Cleaning up network sockets..." << endl;
    closesocket(serverSocket);
    WSACleanup();
    cout << "[System] CyberCore Terminated Safely." << endl;
    return 0;
}