#ifndef READ_METAFILE_H
#define READ_METAFILE_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <map>
#include <vector>
#include "cpp-bencoding/src/bencoding.h"

using namespace std;
class Metafile{   
    public:
    std::string announce, comment, created_by, encoding, name, md5sum;
    vector<vector<string>> announce_list;
    vector<map<string, string>> files;
    vector<string> pieces;
    unsigned int creation_date = 0, i, j, piece_length, private_field = 0, length;
    uint8_t info_hash[20];

    public:
    static Metafile create_metafile(char *path);

    public:
    void print_meta(){
        cout << "Extracted data from meta file:\n";
        if (!announce.empty()){
            cout << "announce: " << announce << "\n";
        }
        if (!created_by.empty()){
            cout << "created by: " << created_by << "\n";
        }
        if (!comment.empty()){
            cout << "comment: " << comment << "\n";
        }
        if (!encoding.empty()){
            cout << "encoding: " << encoding << "\n";
        }
        if (creation_date){
            cout << "creation date: " << creation_date << "\n";
        }
        if (piece_length){
            cout << "piece length: " << piece_length << "\n";
        }
        if (!pieces.empty()){
            cout << "pieces: \n";
            for (int p = 0; p < (int) pieces.size(); p++){
                cout << "\t" << p << ": ";
                for(int o = 0; o < 20; o++){
                    cout << pieces[p][o];
                }
                cout << "\n";
            }
        }
        if (private_field){
            cout << "private: " << private_field << "\n";
        }
        if (!name.empty()){
            cout << "name: " << name << "\n";
        }
        if (length){
            cout << "length: " << length << "\n";
        }
        if (!announce_list.empty()){
            cout << "announce list in a list of lists of strings:\n";
            for (int i = 0; i < (int) announce_list.size(); i++){
                cout << "\tlevel: " << i<< "\n";
                for (int j = 0; j < (int) announce_list[i].size(); j++){
                    cout << "\t\t" << announce_list[i][j] << "\n";
                }
            } 
        }
        if(!files.empty()){
            cout << "files: in a list of dictionaries (map)\n";
            for(i = 0; i < files.size(); i++){
                cout << "\tfile " << i + 1<< ":\n";
                cout << "\t\t" << "length: " << files[i]["length"] << "\n";
                cout << "\t\t" << "md5sum: " << files[i]["md5sum"] << "\n";
                cout << "\t\t" << "path: " << files[i]["path"] << "\n";
            }

        }
        if(info_hash){
            cout << "info-hash: ";
            for (int u = 0; u < 20; u++){
                if (info_hash[u] < 16){
                    printf("0%x", info_hash[u]);
                } else {
                    printf("%x", info_hash[u]);
                }
            }
            cout << "\n";
        }
    }
};

#endif
