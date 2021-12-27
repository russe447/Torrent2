CC=g++
CFLAGS=-Wall -Wextra  -Wformat -Wformat-security -Wno-deprecated-declarations -Wno-long-long -Wmissing-noreturn -Wunreachable-code -fsanitize=address -g -ggdb -O0

all :  read_metafile connector torrent

torrent: torrent.cpp tracker_connector.cpp read_metafile.cpp bencode.o msg_protocol.cpp peer.cpp
	$(CC) $(CFLAGS) torrent.cpp tracker_connector.cpp read_metafile.cpp msg_protocol.cpp peer.cpp -lcurl -fprofile-arcs -ftest-coverage -lgcov -lcheck -lm -lsubunit -lpthread -lrt -lssl -lcrypto bencode.o -o torrent

connector: tracker_connector.cpp tracker_connector.h bencode.o
	$(CC) $(CFLAGS) -D TRACK_TEST tracker_connector.cpp -lcurl -fprofile-arcs -ftest-coverage -lgcov -lcheck -lm -lsubunit -lpthread -lrt bencode.o -o connector

read_metafile: read_metafile.o bencode.o
	$(CC) $(CFLAGS) -o read_metafile read_metafile.o bencode.o -lssl -lcrypto

read_metafile.o: read_metafile.cpp bencode/bencode.h read_metafile.h
	$(CC) $(CFLAGS) -D METAFILE_TEST -c read_metafile.cpp -lssl -lcrypto

bencode.o: bencode/bencode.c bencode/bencode.h
	gcc -Wall -c bencode/bencode.c

clean:
	rm -f *.o *.out *.html *.gcno *.gcda connector read_metafile torrent
