#include <iostream>
#include <fstream>
#include <string.h>
#include <map>
#include <bits/stdc++.h> 
#include <openssl/sha.h>

#include "bencode/bencode.h"
#include "read_metafile.h"
using namespace std;

/*
    Used for announce-list part of the info dictionary. It is an optional arguably useless part of the .torrent file.
    Returns a list of list of strings, each string being an alternate url.
*/
static vector<vector<string>> parse_list_list(be_node **node){
    int i, j;
    vector<vector<string>> list_list;
    for (i = 0; node[i]; ++i){
        vector<string> list;
        for(j = 0; node[i]->val.l[j]; ++j){
            list.push_back((node[i]->val.l[j]->val.s));
        }
        list_list.push_back(list);
    }	
    return list_list;
}
/*  
    Traverses the files part of the info dictionary of the metafile. 
    This is only used if the .torrent file has multiple files that it retrieves. 
    Returns a list of dictionaries, where each dictionary is a file with values: length, md5sum(optional, i think useless), and path 
*/
static vector<map<string, string>> parse_dict(be_node **node){
    int i, j, k;
    vector<map<string, string>> list_dict;
    for (i = 0; node[i]; ++i){
        map<string, string> dict;
        for(j = 0; node[i]->val.d[j].val; ++j){
            if(!strcmp(node[i]->val.d[j].key, "length")){
                dict["length"] = to_string(node[i]->val.d[j].val->val.i);
            } else if(!strcmp(node[i]->val.d[j].key, "md5sum" ) || !strcmp(node[i]->val.d[j].key, "md5")){
                dict["md5sum"] = node[i]->val.d[j].val->val.s;
            } else if(!strcmp(node[i]->val.d[j].key, "path")){
                string path;
                for(k = 0; node[i]->val.d[j].val->val.l[k]; ++k){
                    path = path + node[i]->val.d[j].val->val.l[k]->val.s + "/";
                }
                path.pop_back();
                dict["path"] = path;
            }
        }
        list_dict.push_back(dict);
    }	
    return list_dict;
}

/*
    Opens .torrent file and stores in Metafile object 
    with fields: announce, announce_list (list of list of strings or alt urls), creation_date comment, created_by, encoding,
                 piece_length, pieces (list of sha1 has values), private
                 if the .torrent has only one file: name, length, md5sum
                 if the torrent has more than one file: name, files (list of dictionaries) (will not be NULL) where each dictionary has values length md5sum and path
*/
Metafile Metafile::create_metafile(char *path){
    streampos size;
    char * memblock;
    ifstream torrent;
    be_node *n;
    int i, j, pieces_size;
    size_t pieces_pos;

    torrent.open(path);
    torrent.seekg (0, ios::end);
    size = torrent.tellg();
    memblock = new char [size];
    torrent.seekg(0, ios::beg);
    torrent.read(memblock, size); 
    n = be_decoden(memblock, size);


    //be_dump(n); //Prints metafile in bencode.h's data structure. 
    Metafile meta; 
    for (i = 0; n->val.d[i].val; ++i) {
        if(!strcmp(n->val.d[i].key, "announce")){
            meta.announce = std::string (n->val.d[i].val->val.s);
        } else if (!strcmp(n->val.d[i].key, "announce-list")){
            meta.announce_list = parse_list_list(n->val.d[i].val->val.l);
        } else if (!strcmp(n->val.d[i].key, "creation date")){
            meta.creation_date = n->val.d[i].val->val.i;
        } else if (!strcmp(n->val.d[i].key, "created by")){
            meta.created_by = std::string (n->val.d[i].val->val.s);
        } else if (!strcmp(n->val.d[i].key, "comment")){
            meta.comment = std::string (n->val.d[i].val->val.s);
        } else if (!strcmp(n->val.d[i].key, "encoding")){
            meta.encoding = std::string (n->val.d[i].val->val.s);
        } else if (!strcmp(n->val.d[i].key, "info")){
            string s;
            s.assign(memblock, size);
            size_t info_pos = s.find("info"); 
            pieces_pos = s.find("pieces"); 
            pieces_pos = pieces_pos + 6;
            pieces_size = (atoi(&memblock[pieces_pos]));
            int info_position = (int)info_pos + 4; //finding start of info adds four for "info"
            //finding size of info. Assumes pieces is last element if so then pieces position - info_position + 1 + length given from pieces is the total size
            int info_size = pieces_pos - info_position + pieces_size + 7; 

            char* outBuf = new char[info_size];
            be_encode(n->val.d[i].val, outBuf, info_size);
            SHA1((const unsigned char*)outBuf, info_size, meta.info_hash);
            for (j = 0; n->val.d[i].val->val.d[j].val; ++j) {
                if (!strcmp(n->val.d[i].val->val.d[j].key, "piece length")){
                    meta.piece_length = n->val.d[i].val->val.d[j].val->val.i;
                } else if (!strcmp(n->val.d[i].val->val.d[j].key, "pieces")){
                    /*Finding string length of pieces; cant use strlen because some values in the concatenation of hashes is 0*/
                    // std::string will call strlen() to find the length of memblock, but
                    // using assign() will allow us to specify its length
                    string s;
                    s.assign(memblock, size);
                    size_t pos = s.find("pieces"); 
                    pos = pos + 6;
                    size = (atoi(&memblock[pos]));

                    /*putting each hash of size 20 bytes into element of vector */
                    //char *sha = (char *) malloc(20);
                    // the above line caused a heap buffer overflow
                    char sha[20];
                    for(int k = 0; k < size/20; k++){
                        memcpy(sha, n->val.d[i].val->val.d[j].val->val.s + k*20, 20);
                        // again, we use assign to not have std::string use strlen()
                        // when it adds them to meta.pieces
                        // previously we had a stack overflow when used in torrent
                        string sha_str;
                        sha_str.assign(sha, 20);
                        meta.pieces.push_back(sha_str);
                    }
                } else if (!strcmp(n->val.d[i].val->val.d[j].key, "private")){
                    meta.private_field = n->val.d[i].val->val.d[j].val->val.i;
                } else if(!strcmp(n->val.d[i].val->val.d[j].key, "name")){
                    meta.name = (n->val.d[i].val->val.d[j].val->val.s);
                } else if (!strcmp(n->val.d[i].val->val.d[j].key, "length")){
                    meta.length = n->val.d[i].val->val.d[j].val->val.i;
                } else if (!strcmp(n->val.d[i].val->val.d[j].key, "files")){
                    meta.files = parse_dict(n->val.d[i].val->val.d[j].val->val.l);
                } else if (!strcmp(n->val.d[i].val->val.d[j].key, "md5sum")){
                    meta.md5sum = (n->val.d[i].val->val.d[j].val->val.s);
                }

            }

            delete[] outBuf;
        }
    }
    /*string s = (memblock);
    size_t info_pos = s.find("info"); 
    int info_position = (int)info_pos + 4; //finding start of info adds four for "info"
    //finding size of info. Assumes pieces is last element if so then pieces position - info_position + 1 + length given from pieces is the total size
    int info_size = pieces_pos - info_position + pieces_size + 7; 
    
    char *info_key = (char *)malloc(info_size);
    memcpy(info_key, &memblock[info_position], info_size);
    
            
    SHA1((const unsigned char*)info_key, info_size, meta.info_hash);
    cout << meta.info_hash << "\n";
    for(j=0; j < 20; j++){
        if (meta.info_hash[j] < 16){
            printf("0%x", meta.info_hash[j]);
        } else {
            printf("%x", meta.info_hash[j]);
        }
    }*/
    delete[] memblock;
    //free(n);
    be_free(n);
    return meta;
}
/* 
    To run: ./read_metafile torrent_file.torrent
    creates metafile and prints it 
*/

#ifdef METAFILE_TEST
int main (int argc, char* argv[]) {
    Metafile meta = Metafile::create_metafile(argv[argc - 1]);
    meta.print_meta(); //prints metafile: see read_metafile.h
    return 0;
}
#endif
 
