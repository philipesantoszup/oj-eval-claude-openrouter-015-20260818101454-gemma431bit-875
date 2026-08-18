#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

const char* DB_FILE = "storage.db";
const int NUM_BUCKETS = 1000000;

struct Entry {
    char index[64];
    uint8_t index_len;
    int value;
    long long next;
    bool deleted;
};

unsigned long hash_index(const string& s) {
    unsigned long hash = 14695981039346656037ULL;
    for (char c : s) {
        hash ^= (unsigned char)c;
        hash *= 1099511628211ULL;
    }
    return hash % NUM_BUCKETS;
}

class FileStorage {
    int fd;
    void* map_ptr;
    size_t map_size;

    void extend_file(size_t new_size) {
        if (ftruncate(fd, new_size) == -1) {
            // Handle error
        }
        if (map_ptr) munmap(map_ptr, map_size);
        map_ptr = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        map_size = new_size;
    }

public:
    FileStorage() {
        fd = open(DB_FILE, O_RDWR | O_CREAT, 0644);
        struct stat st;
        fstat(fd, &st);
        if (st.st_size == 0) {
            size_t initial_size = NUM_BUCKETS * sizeof(long long);
            ftruncate(fd, initial_size);
            map_ptr = mmap(NULL, initial_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            map_size = initial_size;
            long long* buckets = (long long*)map_ptr;
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                buckets[i] = -1;
            }
        } else {
            map_ptr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            map_size = st.st_size;
        }
    }

    ~FileStorage() {
        if (map_ptr) munmap(map_ptr, map_size);
        if (fd != -1) close(fd);
    }

    void insert(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long* buckets = (long long*)map_ptr;
        long long current_offset = buckets[h];
        long long prev_offset = -1;

        while (current_offset != -1) {
            if (current_offset >= (long long)map_size) break;
            Entry* e = (Entry*)((char*)map_ptr + current_offset);
            if (!e->deleted && e->index_len == index.length() && memcmp(e->index, index.c_str(), e->index_len) == 0) {
                if (e->value == value) return;
            }
            prev_offset = current_offset;
            current_offset = e->next;
        }

        size_t new_offset = map_size;
        if (new_offset < (size_t)NUM_BUCKETS * sizeof(long long)) {
            new_offset = (size_t)NUM_BUCKETS * sizeof(long long);
        }

        if (new_offset + sizeof(Entry) > map_size) {
            extend_file(map_size + (1024 * 1024));
            buckets = (long long*)map_ptr;
        }

        Entry* new_entry = (Entry*)((char*)map_ptr + new_offset);
        memset(new_entry->index, 0, 64);
        memcpy(new_entry->index, index.c_str(), min((int)index.length(), 64));
        new_entry->index_len = (uint8_t)min((int)index.length(), 64);
        new_entry->value = value;
        new_entry->next = -1;
        new_entry->deleted = false;

        if (prev_offset == -1) {
            buckets[h] = new_offset;
        } else {
            Entry* prev_e = (Entry*)((char*)map_ptr + prev_offset);
            prev_e->next = new_offset;
        }
    }

    void remove(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long* buckets = (long long*)map_ptr;
        long long current_offset = buckets[h];
        long long prev_offset = -1;

        while (current_offset != -1) {
            if (current_offset >= (long long)map_size) break;
            Entry* e = (Entry*)((char*)map_ptr + current_offset);
            if (!e->deleted && e->index_len == index.length() && memcmp(e->index, index.c_str(), e->index_len) == 0) {
                if (e->value == value) {
                    e->deleted = true;
                    if (prev_offset == -1) {
                        buckets[h] = e->next;
                    } else {
                        Entry* prev_e = (Entry*)((char*)map_ptr + prev_offset);
                        prev_e->next = e->next;
                    }
                    return;
                }
            }
            prev_offset = current_offset;
            current_offset = e->next;
        }
    }

    void find(const string& index) {
        unsigned long h = hash_index(index);
        long long* buckets = (long long*)map_ptr;
        long long current_offset = buckets[h];
        vector<int> results;

        while (current_offset != -1) {
            if (current_offset >= (long long)map_size) break;
            Entry* e = (Entry*)((char*)map_ptr + current_offset);
            if (!e->deleted && e->index_len == index.length() && memcmp(e->index, index.c_str(), e->index_len) == 0) {
                results.push_back(e->value);
            }
            current_offset = e->next;
        }

        if (results.empty()) {
            printf("null\n");
        } else {
            sort(results.begin(), results.end());
            for (int i = 0; i < results.size(); ++i) {
                printf("%d%c", results[i], (i == results.size() - 1 ? '\n' : ' '));
            }
        }
    }
};

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    FileStorage fs;
    char cmd[20], index[100];
    int value;

    for (int i = 0; i < n; ++i) {
        scanf("%s", cmd);
        if (strcmp(cmd, "insert") == 0) {
            scanf("%s %d", index, &value);
            fs.insert(index, value);
        } else if (strcmp(cmd, "delete") == 0) {
            scanf("%s %d", index, &value);
            fs.remove(index, value);
        } else if (strcmp(cmd, "find") == 0) {
            scanf("%s", index);
            fs.find(index);
        }
    }

    return 0;
}
