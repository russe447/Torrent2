#ifndef TORRENT_H
#define TORRENT_H

#include <string>

#include "read_metafile.h"
#include "tracker_connector.h"

class Torrent
{
    std::string make_peer_id();
public:
    Metafile meta;
    //TrackerConnector connector;

    Torrent(char* metafile_path, int port);
};

#endif
