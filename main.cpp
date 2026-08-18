#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

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
    FILE* fp;

public:
    FileStorage() {
        fp = fopen(DB_FILE, "rb+");
        if (!fp) {
            fp = fopen(DB_FILE, "wb+");
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                long long val = -1;
                fwrite(&val, sizeof(long long), 1, fp);
            }
            fflush(fp);
        }
        setvbuf(fp, NULL, _IOFBF, 1024 * 1024); // 1MB buffer
    }

    ~FileStorage() {
        if (fp) fclose(fp);
    }

    void insert(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long current_offset;
        fseek(fp, h * sizeof(long long), SEEK_SET);
        fread(&current_offset, sizeof(long long), 1, fp);

        long long prev_offset = -1;

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                if (e.value == value) return;
            }
            prev_offset = current_offset;
            current_offset = e.next;
        }

        fseek(fp, 0, SEEK_END);
        long long new_offset = ftell(fp);
        Entry new_entry;
        memset(new_entry.index, 0, 64);
        memcpy(new_entry.index, index.c_str(), min((int)index.length(), 64));
        new_entry.index_len = (uint8_t)min((int)index.length(), 64);
        new_entry.value = value;
        new_entry.next = -1;
        new_entry.deleted = false;

        fseek(fp, new_offset, SEEK_SET);
        fwrite(&new_entry, sizeof(Entry), 1, fp);

        if (prev_offset == -1) {
            fseek(fp, h * sizeof(long long), SEEK_SET);
            fwrite(&new_offset, sizeof(long long), 1, fp);
        } else {
            fseek(fp, prev_offset + offsetof(Entry, next), SEEK_SET);
            fwrite(&new_offset, sizeof(long long), 1, fp);
        }
    }

    void remove(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long current_offset;
        fseek(fp, h * sizeof(long long), SEEK_SET);
        fread(&current_offset, sizeof(long long), 1, fp);

        long long prev_offset = -1;

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                if (e.value == value) {
                    fseek(fp, current_offset + offsetof(Entry, deleted), SEEK_SET);
                    bool del = true;
                    fwrite(&del, sizeof(bool), 1, fp);

                    if (prev_offset == -1) {
                        fseek(fp, h * sizeof(long long), SEEK_SET);
                        fwrite(&e.next, sizeof(long long), 1, fp);
                    } else {
                        fseek(fp, prev_offset + offsetof(Entry, next), SEEK_SET);
                        fwrite(&e.next, sizeof(long long), 1, fp);
                    }
                    return;
                }
            }
            prev_offset = current_offset;
            current_offset = e.next;
        }
    }

    void find(const string& index) {
        unsigned long h = hash_index(index);
        long long current_offset;
        fseek(fp, h * sizeof(long long), SEEK_SET);
        fread(&current_offset, sizeof(long long), 1, fp);

        vector<int> results;

        while (current_offset != -1) {
            fseek(fp, current_offset, SEEK_SET);
            Entry e;
            fread(&e, sizeof(Entry), 1, fp);
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                results.push_back(e.value);
            }
            current_offset = e.next;
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
