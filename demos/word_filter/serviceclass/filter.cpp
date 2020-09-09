
#include "bidirservice.hpp"
#include "fixedstring.hpp"

#include <libpirate.h>
#include <pal/pal.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#pragma pirate enclave declare(filter_ui_sc)
#pragma pirate enclave declare(filter_host_sc)
#pragma pirate capability declare(sensitive_words)
#pragma pirate enclave capability(filter_host_sc, sensitive_words)

#define ARRAY_LEN(arr) (sizeof arr / sizeof *arr)

namespace {

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

using Req = FixedString<80>;
using Rsp = FixedString<80>;
struct CensorService : public BidirService<CensorService, Req, Rsp> {
    Rsp impl(Req str) {
        censor(str.str);
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

        line = service(line);

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
