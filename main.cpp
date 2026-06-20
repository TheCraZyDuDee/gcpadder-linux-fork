#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <memory.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <limits>

struct NETPADData {
  uint16_t buttons;
  int8_t stickX;
  int8_t stickY;
  int8_t substickX;
  int8_t substickY;
  uint8_t triggerL;
  uint8_t triggerR;
} paddata;

enum {
  A = 0x0001,
  B = 0x0002,
  X = 0x0004,
  Y = 0x0008,
  START = 0x0010,
  D_LEFT = 0x0100,
  D_RIGHT = 0x0200,
  D_DOWN = 0x0400,
  D_UP = 0x0800,
  Z = 0x1000,
  TRIG_R = 0x2000,
  TRIG_L = 0x4000
} btn_values;

#define PAYLOAD "\x09\xAD\x09\xAD"

struct Config {
  std::string ip;
  std::string port;
};

std::string trim(const std::string& s) {
  auto start = s.begin();
  while (start != s.end() && std::isspace(*start)) ++start;
  auto end = s.end();
  while (end != start && std::isspace(*(end-1))) --end;
  return std::string(start, end);
}

Config load_config() {
  Config cfg;
  std::ifstream file("gamecube_controller.ini");
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.find("IP=") == 0) {
        cfg.ip = trim(line.substr(3));
      } else if (line.find("PORT=") == 0) {
        cfg.port = trim(line.substr(5));
      }
    }
    file.close();
    std::cout << "Loaded config: IP='" << cfg.ip << "', PORT='" << cfg.port << "'" << std::endl;
  } else {
    std::cout << "No config file found." << std::endl;
  }
  return cfg;
}

void save_config(const Config& cfg) {
  std::ofstream file("gamecube_controller.ini");
  if (file.is_open()) {
    file << "IP=" << trim(cfg.ip) << "\n";
    file << "PORT=" << trim(cfg.port) << "\n";
    file.close();
    std::cout << "Saved config: IP='" << trim(cfg.ip) << "', PORT='" << trim(cfg.port) << "'" << std::endl;
  } else {
    std::cerr << "Warning: Could not save config file.\n";
  }
}

Config prompt_user() {
  Config cfg;
  std::cout << "Enter the IP address of the Wii Console: ";
  std::getline(std::cin, cfg.ip);

  std::cout << "Enter the port (default 2477): ";
  std::string portInput;
  std::getline(std::cin, portInput);
  if (portInput.empty()) {
    cfg.port = "2477";
  } else {
    cfg.port = portInput;
  }
  return cfg;
}

int resolve_helper(const char* hostname, int family, const char* service,
                   sockaddr_storage* pAddr) {
  int result;
  addrinfo* result_list = NULL;
  addrinfo hints = {};
  hints.ai_family = family;
  hints.ai_socktype = SOCK_DGRAM;
  result = getaddrinfo(hostname, service, &hints, &result_list);
  if (result == 0) {
    memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
    freeaddrinfo(result_list);
  }
  return result;
}

int main(int argc, char* argv[]) {
  int result = 0;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  Config config = load_config();
  if (config.ip.empty()) {
    config = prompt_user();
    save_config(config);
  }
  std::cout << "Connecting to " << config.ip << ":" << config.port << std::endl;

  struct input_absinfo absx;
  absx.flat = 10;
  absx.fuzz = 0;
  absx.value = 0;
  absx.minimum = -100;
  absx.maximum = 100;
  absx.resolution = 0;
  struct input_absinfo absy;
  absy.flat = 10;
  absy.fuzz = 0;
  absy.value = 0;
  absy.minimum = -100;
  absy.maximum = 100;
  absy.resolution = 0;
  struct input_absinfo absrx;
  absrx.flat = 10;
  absrx.fuzz = 0;
  absrx.value = 0;
  absrx.minimum = -100;
  absrx.maximum = 100;
  absrx.resolution = 0;
  struct input_absinfo absry;
  absry.flat = 10;
  absry.fuzz = 0;
  absry.value = 0;
  absry.minimum = -100;
  absry.maximum = 100;
  absry.resolution = 0;

  struct input_absinfo absz;
  absz.flat = 0;
  absz.fuzz = 0;
  absz.value = 0;
  absz.minimum = 0;
  absz.maximum = 200;
  absz.resolution = 0;
  struct input_absinfo absrz;
  absrz.flat = 0;
  absrz.fuzz = 0;
  absrz.value = 0;
  absrz.minimum = 0;
  absrz.maximum = 200;
  absrz.resolution = 0;

  struct libevdev* dev = libevdev_new();
  struct libevdev_uinput* uidev;
  libevdev_set_name(dev, "Nintendo GameCube Controller");

  libevdev_enable_event_type(dev, EV_ABS);

  libevdev_enable_event_code(dev, EV_ABS, ABS_X, &absx);
  libevdev_set_abs_info(dev, ABS_X, &absx);
  libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &absy);
  libevdev_set_abs_info(dev, ABS_Y, &absy);
  libevdev_enable_event_code(dev, EV_ABS, ABS_RX, &absrx);
  libevdev_set_abs_info(dev, ABS_RX, &absrx);
  libevdev_enable_event_code(dev, EV_ABS, ABS_RY, &absry);
  libevdev_set_abs_info(dev, ABS_RY, &absry);

  libevdev_enable_event_code(dev, EV_ABS, ABS_Z, &absz);
  libevdev_set_abs_info(dev, ABS_Z, &absz);
  libevdev_enable_event_code(dev, EV_ABS, ABS_RZ, &absrz);
  libevdev_set_abs_info(dev, ABS_RZ, &absrz);

  libevdev_enable_event_type(dev, EV_KEY);

  libevdev_enable_event_code(dev, EV_KEY, BTN_SOUTH, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_EAST, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_NORTH, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_WEST, NULL);

  libevdev_enable_event_code(dev, EV_KEY, BTN_TR, NULL);

  libevdev_enable_event_code(dev, EV_KEY, BTN_TL2, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_TR2, NULL);

  libevdev_enable_event_code(dev, EV_KEY, BTN_START, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_UP, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_DOWN, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_LEFT, NULL);
  libevdev_enable_event_code(dev, EV_KEY, BTN_DPAD_RIGHT, NULL);

  result = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                              &uidev);
  if (result != 0) {
    int lasterror = errno;
    std::cout << "error on creating uinput device: " << strerror(lasterror);
    exit(1);
  }

  sockaddr_in addrListen = {};
  addrListen.sin_family = AF_INET;
  result = bind(sock, (sockaddr*)&addrListen, sizeof(addrListen));
  if (result == -1) {
    int lasterror = errno;
    std::cout << "error: " << lasterror;
    exit(1);
  }

  sockaddr_storage addrDest = {};
  result = resolve_helper(config.ip.c_str(), AF_INET, config.port.c_str(), &addrDest);
  if (result != 0) {
    int lasterror = errno;
    std::cout << "error: " << lasterror;
    exit(1);
  }

  result = sendto(sock, PAYLOAD, 4, 0, (sockaddr*)&addrDest, sizeof(addrDest));
  if (result != 4) {
    int lasterror = errno;
    std::cout << "error: " << lasterror;
    exit(1);
  }

  std::cout << result << " bytes sent" << std::endl;

  uint16_t prevButtons = 0xFFFF;

  while (true) {
    result = recv(sock, &paddata, 8, 0);
    if (result == -1) {
      int lasterror = errno;
      std::cout << "error: " << lasterror;
      exit(1);
    }

    if (paddata.buttons != prevButtons) {
      std::cout << "Buttons: " << paddata.buttons << "\n";
      prevButtons = paddata.buttons;
    }

    libevdev_uinput_write_event(uidev, EV_ABS, ABS_X, paddata.stickX);
    libevdev_uinput_write_event(uidev, EV_ABS, ABS_Y, -paddata.stickY);
    libevdev_uinput_write_event(uidev, EV_ABS, ABS_RX, paddata.substickX);
    libevdev_uinput_write_event(uidev, EV_ABS, ABS_RY, -paddata.substickY);
    libevdev_uinput_write_event(uidev, EV_ABS, ABS_Z, paddata.triggerL);
    libevdev_uinput_write_event(uidev, EV_ABS, ABS_RZ, paddata.triggerR);

    libevdev_uinput_write_event(uidev, EV_KEY, BTN_SOUTH,
                                (paddata.buttons & A) == A ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_EAST,
                                (paddata.buttons & B) == B ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_NORTH,
                                (paddata.buttons & X) == X ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_WEST,
                                (paddata.buttons & Y) == Y ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR,
                                (paddata.buttons & Z) == Z ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2,
                                (paddata.buttons & TRIG_L) == TRIG_L ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2,
                                (paddata.buttons & TRIG_R) == TRIG_R ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_START,
                                (paddata.buttons & START) == START ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_DPAD_UP,
                                (paddata.buttons & D_UP) == D_UP ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_DPAD_DOWN,
                                (paddata.buttons & D_DOWN) == D_DOWN ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_DPAD_LEFT,
                                (paddata.buttons & D_LEFT) == D_LEFT ? 1 : 0);
    libevdev_uinput_write_event(uidev, EV_KEY, BTN_DPAD_RIGHT,
                                (paddata.buttons & D_RIGHT) == D_RIGHT ? 1 : 0);

    libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
  }

  return 0;
}