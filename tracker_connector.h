#include <curl/curl.h>
#include <string>

#include "bencode/bencode.h"

struct peer
{
    std::string ip;
    std::string peer_id;
    int port;
};

class TrackerConnector
{
    CURL* curl;
    std::string url;
    std::vector<peer> peers;

    // do not call this function directly. It is a callback for libcurl
    static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
    std::vector<peer> parse_list_dict(be_node** node);

    public:
        std::string encode(uint8_t* info_hash, uint8_t* peer_id, int port);

        TrackerConnector(std::string url);
        TrackerConnector();
        ~TrackerConnector();
        int request(uint8_t* info_hash, uint8_t* peer_id, int port);
        std::vector<peer> get_peers();
};
