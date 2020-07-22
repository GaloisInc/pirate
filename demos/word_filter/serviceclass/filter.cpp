#include <libpirate.h>
#include <pal/pal.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#pragma pirate enclave declare(filter_ui_sc)
#pragma pirate enclave declare(filter_host_sc)
#pragma pirate capability declare(sensitive_words)
#pragma pirate enclave capability(filter_host_sc, sensitive_words)

#define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

pirate_channel ui_to_host
    __attribute__((pirate_resource("ui_to_host", "filter_ui_sc")))
    __attribute__((pirate_resource_param("permissions", "writeonly", "filter_ui_sc")))
    __attribute__((pirate_resource("ui_to_host", "filter_host_sc")))
    __attribute__((pirate_resource_param("permissions", "readonly", "filter_host_sc")));

pirate_channel host_to_ui
    __attribute__((pirate_resource("host_to_ui", "filter_host_sc")))
    __attribute__((pirate_resource_param("permissions", "writeonly", "filter_host_sc")))
    __attribute__((pirate_resource("host_to_ui", "filter_ui_sc")))
    __attribute__((pirate_resource_param("permissions", "readonly", "filter_ui_sc")));

namespace {

////////////////////////////////////////////////////////////////////////
// Serialization library
////////////////////////////////////////////////////////////////////////

template<typename T> struct Serialize {};
template<> struct Serialize<std::string> {
    static std::string fromBuffer(std::vector<char> const& buffer) {
        return std::string(buffer.begin(), buffer.end());
    }
    static void toBuffer(std::vector<char> & buffer, std::string const& str) {
        buffer.clear();
        buffer.insert(buffer.end(), str.begin(), str.end());
    }
};

////////////////////////////////////////////////////////////////////////
// Bi-directional Service library
////////////////////////////////////////////////////////////////////////

template<typename Derived, typename T, typename U>
class Service {
    int readChan;
    int writeChan;

    void write_all(char const* buf, size_t count) {
      size_t sofar = 0;
      while (sofar < count) {
        ssize_t result = pirate_write(writeChan, buf + sofar, count - sofar);
        if (result < 0) {
          perror("pirate_write");
          exit(EXIT_FAILURE);
        }
        sofar += result;
      }
    }

    void read_all(char * buf, size_t count) {
      size_t sofar = 0;
      while (sofar < count) {
        ssize_t result = pirate_read(readChan, buf + sofar, count - sofar);
        if (result < 0) {
          perror("pirate_read");
          exit(EXIT_FAILURE);
        }
        sofar += result;
      }
    }

    void transmit(char const* msg, size_t n) {
        write_all((char const*)&n, sizeof n);
        write_all(msg, n);
    }

    void receive(std::vector<char> &buffer)
    {
      size_t n;
      read_all((char *)&n, sizeof n);
      buffer.resize(n);
      read_all(buffer.data(), n);
    }

    inline U interface(T t) {
	    return static_cast<Derived*>(this)->impl(t);
    }

public:
    void setHandles(int read, int write) { readChan = read; writeChan = write; }

    U call(T t) {
        std::vector<char> buffer;
        Serialize<T>::toBuffer(buffer, t);
        transmit(buffer.data(), buffer.size());
        buffer.clear();
        receive(buffer);
        return Serialize<U>::fromBuffer(buffer);
    }

    int event_loop() {
        std::vector<char> buffer;
        for (;;) {
            buffer.clear();
            receive(buffer);
            U req = Serialize<T>::fromBuffer(buffer);
            auto res = interface(req);
            buffer.clear();
            Serialize<U>::toBuffer(buffer, res);
            transmit(buffer.data(), buffer.size());
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Word filter demonstration
////////////////////////////////////////////////////////////////////////

const char *word_list[]
__attribute__((pirate_capability("sensitive_words")))
= {
  "agile", "disruptive", "ecosystem", "incentivize",
  "low-hanging fruit", "negative growth", "paradigm shift",
  "rightsizing", "synergies",
};

void censor(std::string &msg)
{
  for (auto const word : word_list) {
    auto const n = strlen(word);
    
    for(std::string::size_type pos = 0;;) {
        pos = msg.find(word, pos);
        if (std::string::npos == pos) { break; }
        msg.replace(pos, n, n, '*');
    }
  }
}

struct CensorService : public Service<CensorService, std::string, std::string> {
    std::string impl(std::string str) {
        censor(str);
        return str;
    }
} service; // __attribute__((pirate_resource(...)))

} // namespace

/* Main function for the user interface.
 */
int ui(void)
__attribute__((pirate_enclave_main("filter_ui_sc")))
{
    service.setHandles(host_to_ui, ui_to_host);

    /* Begin actual code */
    std::string line;

    for(;;) {
        std::cout << "Input> ";
        std::flush(std::cout);

        line.clear();
        std::getline(std::cin, line);
        if (!std::cin) { break; }

        line = service.call(line);

        std::cout << "Response> " << line << std::endl;
  }

  return 0;
}

int host(void)
__attribute__((pirate_enclave_main("filter_host_sc")))
{
  service.setHandles(ui_to_host, host_to_ui);
  return service.event_loop();
}
