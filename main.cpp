#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cstdint>

using namespace std;

const string DB_FILE = "storage.db";
const int NUM_BUCKETS = 250000;

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
    fstream file;
    long long buckets[NUM_BUCKETS];

public:
    FileStorage() {
        file.open(DB_FILE, ios::in | ios::out | ios::binary);
        if (!file) {
            file.open(DB_FILE, ios::out | ios::binary);
            for (int i = 0; i < NUM_BUCKETS; ++i) {
                long long val = -1;
                file.write(reinterpret_cast<char*>(&val), sizeof(long long));
            }
            file.close();
            file.open(DB_FILE, ios::in | ios::out | ios::binary);
        }

        for (int i = 0; i < NUM_BUCKETS; ++i) {
            file.read(reinterpret_cast<char*>(&buckets[i]), sizeof(long long));
        }
    }

    ~FileStorage() {
        if (file.is_open()) {
            file.close();
        }
    }

    void insert(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long current_offset = buckets[h];
        long long prev_offset = -1;

        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                if (e.value == value) return; // Already exists
            }
            prev_offset = current_offset;
            current_offset = e.next;
        }

        // Append new entry
        file.seekg(0, ios::end);
        long long new_offset = file.tellg();
        Entry new_entry;
        memset(new_entry.index, 0, 64);
        memcpy(new_entry.index, index.c_str(), min((int)index.length(), 64));
        new_entry.index_len = (uint8_t)min((int)index.length(), 64);
        new_entry.value = value;
        new_entry.next = -1;
        new_entry.deleted = false;

        file.seekp(new_offset);
        file.write(reinterpret_cast<char*>(&new_entry), sizeof(Entry));

        if (prev_offset == -1) {
            buckets[h] = new_offset;
            file.seekp(h * sizeof(long long));
            file.write(reinterpret_cast<char*>(&buckets[h]), sizeof(long long));
        } else {
            file.seekp(prev_offset + offsetof(Entry, next));
            file.write(reinterpret_cast<char*>(&new_offset), sizeof(long long));
        }
    }

    void remove(const string& index, int value) {
        unsigned long h = hash_index(index);
        long long current_offset = buckets[h];
        long long prev_offset = -1;

        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                if (e.value == value) {
                    // Mark as deleted
                    file.seekp(current_offset + offsetof(Entry, deleted));
                    bool del = true;
                    file.write(reinterpret_cast<char*>(&del), sizeof(bool));

                    // Optionally update previous entry's next pointer
                    if (prev_offset == -1) {
                        buckets[h] = e.next;
                        file.seekp(h * sizeof(long long));
                        file.write(reinterpret_cast<char*>(&buckets[h]), sizeof(long long));
                    } else {
                        file.seekp(prev_offset + offsetof(Entry, next));
                        file.write(reinterpret_cast<char*>(&e.next), sizeof(long long));
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
        long long current_offset = buckets[h];
        vector<int> results;

        while (current_offset != -1) {
            file.seekg(current_offset);
            Entry e;
            file.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (!e.deleted && e.index_len == index.length() && memcmp(e.index, index.c_str(), e.index_len) == 0) {
                results.push_back(e.value);
            }
            current_offset = e.next;
        }

        if (results.empty()) {
            cout << "null" << endl;
        } else {
            sort(results.begin(), results.end());
            for (int i = 0; i < results.size(); ++i) {
                cout << results[i] << (i == results.size() - 1 ? "" : " ");
            }
            cout << endl;
        }
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    FileStorage fs;
    string cmd, index;
    int value;

    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            cin >> index >> value;
            fs.insert(index, value);
        } else if (cmd == "delete") {
            cin >> index >> value;
            fs.remove(index, value);
        } else if (cmd == "find") {
            cin >> index;
            fs.find(index);
        }
    }

    return 0;
}
