#include <arpa/inet.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>

#include "tracker_connector.h"
#include "bencode/bencode.h"

// if you see "error: ‘std::istringstream {aka class std::__cxx11::basic_istringstream<char>}’ 
// has no member named ‘_ck_assert_failed’ "
// when compiling, it's probably because of check.h

#ifdef TRACK_TEST
#include <check.h>
#endif

std::vector<peer> TrackerConnector::parse_list_dict(be_node **node)
{
    std::vector<peer> peer_list;

    for (unsigned i = 0; node[i]; ++i)
    {
        peer p;

        std::string str;
        str.assign(node[i]->val.d[0].key, 2);

        if (str != "ip")
            break;

        p.ip = node[i]->val.d[0].val->val.s;
        p.peer_id = node[i]->val.d[1].val->val.s;
        p.port = node[i]->val.d[2].val->val.i;

        peer_list.push_back(p);
    }

    return peer_list;
}


// this is the callback for curl to read the tracker response. Do not call this directly.
size_t TrackerConnector::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::cout << "in callback" << std::endl;
    //std::cout << ptr << std::endl;

    memcpy(userdata, ptr, nmemb * size);

    return size * nmemb;
}

TrackerConnector::TrackerConnector()
{

}

//construct the http get request
std::string TrackerConnector::encode(uint8_t* info_hash, uint8_t* peer_id, int port)
{
    char* hash_encoded = curl_easy_escape(curl, (char*)info_hash, 20);
    char* peer_encoded = curl_easy_escape(curl, (char*)peer_id, 20);

    std::string get_request;

    char port_str[100];
    memset(port_str, 0, sizeof(port_str));
    sprintf(port_str, "%d", port);

    std::cout << "the url is" << std::endl;
    std::cout << url << std::endl;
    get_request.append(url);
    get_request.append("?");

    get_request.append("info_hash=");
    get_request.append(hash_encoded);
    get_request.append("&");

    get_request.append("peer_id=");
    get_request.append(peer_encoded);
    get_request.append("&");

    get_request.append("port=");
    get_request.append(port_str);
    get_request.append("&");

    get_request.append("uploaded=0");
    get_request.append("&");

    get_request.append("downloaded=0");
    get_request.append("&");

    get_request.append("left=0");
    get_request.append("&");

    // loushao.net:8080 gives non-compact form
    get_request.append("compact=0");
    
    curl_free(hash_encoded);
    curl_free(peer_encoded);

    std::cout << "constructed the following get request" << std::endl;
    std::cout << get_request << std::endl;

    return get_request;
}

// this function assumes the info_hash and peer_id are both correctly
// initialized to a 20 byte length
// perform a server request
int TrackerConnector::request(uint8_t* info_hash, uint8_t* peer_id, int port)
{
    if (info_hash == nullptr)
    {
        std::cout << "info hash is nullptr" << std::endl;

        return -1;
    }

    if (peer_id == nullptr)
    {
        std::cout << "peer id is nullptr" << std::endl;

        return -1;
    }

    char response[10000];
    memset(response, 0, sizeof(response));

    curl_easy_setopt(curl, CURLOPT_URL, encode(info_hash, peer_id, port).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_perform(curl);

    std::cout << "sent request to tracker. tracker response is: " << std::endl;
    std::cout << response << std::endl;

    std::string response_str(response);
    //std::cout << "bencode decoding the response: " << std::endl;

    //auto data = bencoding::decode(response_str);
    //std::string repr = bencoding::getPrettyRepr(std::move(data));

    be_node* n = be_decode(response);

    for (unsigned i = 0; n->val.d[i].val; i++)
        if (strcmp(n->val.d[i].key, "peers") == 0)
            peers = parse_list_dict(n->val.d[i].val->val.l);            

/*
    for (unsigned i = 0; i < peers.size(); i++)
    {
        std::cout << peers[i].ip << std::endl;
        std::cout << peers[i].port << std::endl;
        std::cout << peers[i].peer_id << std::endl;
    }
*/

    be_free(n);

    return 0;
}

std::vector<peer> TrackerConnector::get_peers()
{
    return peers;
}

// initializes curl and stores the url of the tracker
TrackerConnector::TrackerConnector(std::string trackerurl)
{
    curl = curl_easy_init();

    if (curl == NULL)
    {
        std::cout << "curl init failed" << std::endl;

        return;
    }

    url = trackerurl;
}

// frees curl
TrackerConnector::~TrackerConnector()
{
    curl_easy_cleanup(curl);

    curl_global_cleanup();
}

// this is testing code. You need libcheck to run it. 
// To run it, put "-D TRACK_TEST" on the command line when you compile it.
#ifdef TRACK_TEST

START_TEST(encode_1)
{
    uint8_t* hash = (uint8_t*)"fhgk3b68xkdjfi3iu8d9";
    uint8_t* peer = (uint8_t*)"hgk3b68xkdjfi3i500d0";

    //TrackerConnector track("tracker.opentrackr.org:1337/announce");
    //TrackerConnector track((char*)"tracker.opentrackr.org");
    TrackerConnector track((char*)"www.loushao.net");
    
    std::string encoding = track.encode(hash, peer, 27015);
    std::cout << encoding << std::endl;

    ck_assert_str_eq("http://www.loushao.net:8080/announce?info_hash=fhgk3b68xkdjfi3iu8d9&peer_id=hgk3b68xkdjfi3i500d0&port=27015&uploaded=0&downloaded=0&left=0&compact=0", (const char*)encoding.c_str());

    track.request(hash, peer, 27015);
}
END_TEST

Suite *track_test_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("track test");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, encode_1);
    suite_add_tcase(s, tc_core);

    return s;
}

int main()
{
    int no_failed = 0;
    Suite *s;
    SRunner *runner;

    s = track_test_suite();
    runner = srunner_create(s);

    srunner_run_all(runner, CK_NORMAL);
    no_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (no_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

    return 0;
}

#endif
