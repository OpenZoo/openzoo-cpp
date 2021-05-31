#ifndef __UTILS_TOKENMAP_H__
#define __UTILS_TOKENMAP_H__

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ZZT {

    template<typename E, E* defaultValue, bool copyTokenNames>
    class TokenCopyMap;

    template<typename E, E defaultValue, bool copyTokenNames>
    class TokenMap {
        const char **tokens;
        E *values;
        int16_t count, size;

        bool resize(int16_t new_size) {
            const char **new_tokens = new const char*[new_size];
            E *new_values = new E[new_size];
            if (new_tokens == NULL || new_values == NULL) {
                return false;
            }
            if (size > 0) {
                memcpy(new_tokens, tokens, sizeof(const char*) * size);
                memcpy(new_values, values, sizeof(E) * size);
                delete tokens;
                delete values;
            }
            tokens = new_tokens;
            values = new_values;
            size = new_size;
            return true;
        }

         template<typename F, F* fDefaultValue, bool fCopyTokenNames>
         friend class TokenCopyMap;

    public:
        TokenMap(): TokenMap(4) { }
        TokenMap(int16_t initial_size) {
            count = 0;
            size = 0;
            resize(initial_size);
        }

        void clear(void) {
            if (copyTokenNames) {
                for (int i = 0; i < count; i++) {
                    free((void*) tokens[i]);
                }
            }
            count = 0;
            // TODO: make smaller?
        }

        void add(const char *key, E value, bool override) {
            if (count >= size) {
                resize(size << 1);
            }

            if (!override) {
                for (int16_t pos = 0; pos < count; pos++) {
                    if (!strcmp(tokens[pos], key)) {
                        return;
                    }
                }
            }

            int16_t pos = count - 1;
            while (pos >= 0) {
                int result = strcmp(tokens[pos], key);
                if (result > 0) {
                    tokens[pos + 1] = tokens[pos];
                    values[pos + 1] = values[pos];
                    pos--;
                } else {
                    break;
                }
            }

            if (copyTokenNames) {
                int keyLen = strlen(key) + 1;
                char *newKey = (char*) malloc(keyLen);
                memcpy(newKey, key, keyLen);
                tokens[pos + 1] = newKey;
            } else {
                tokens[pos + 1] = key;
            }
            values[pos + 1] = value;
            count++;
        }

        inline void add(const char *key, E value) {
            add(key, value, false);
        }

        E get(const char *key) const {
            int start = 0;
            int end = count - 1;
            int pos = (start + end) >> 1;

            while (start <= end) {
                int result = strcmp(tokens[pos], key);
                if (result == 0) {
                    return values[pos];
                } else if (result < 0) {
                    start = pos + 1;
                } else {
                    end = pos - 1;
                }
                pos = (start + end) >> 1;
            }

            return defaultValue;
        }

        void remove(const char *key) {
            bool found = false;

            for (int i = 0; i < count; i++) {
                if (found) {
                    tokens[i - 1] = tokens[i];
                    values[i - 1] = values[i];
                } else if(!strcmp(tokens[i], key)) {
                    found = true;
                    if (copyTokenNames) {
                        free((void*) tokens[i]);
                    }
                }
            }

            if (found) {
                count--;
            }
        }

        void debug_print(void) {
            for (int i = 0; i < count; i++) {
                fprintf(stderr, "%d: %s\n", i, tokens[i]);
            }
        }
    };
    
    template<typename E, E* defaultValue, bool copyTokenNames>
    class TokenCopyMap {
        TokenMap<E*, defaultValue, copyTokenNames> map;

    public:
        TokenCopyMap(): TokenCopyMap(4) { }
        TokenCopyMap(int16_t initial_size): map(initial_size) { }

        void clear(void) {
            for (int i = 0; i < map.count; i++) {
                free(map.values[i]);
            }
            map.count = 0;
        }

        void add(const char *key, E* value, size_t value_size, bool override) {
            // TODO: memory safety
            E* newValue = malloc(value_size);
            memcpy(newValue, value, value_size);
            map.add(key, newValue, override);
        }

        void add(const char *key, E* value, size_t value_size) {
            add(key, value, value_size, false);
        }

        inline E* get(const char *key) const {
            return map.get(key);
        }

        void remove(const char *key) {
            bool found = false;

            for (int i = 0; i < map.count; i++) {
                if (found) {
                    map.tokens[i - 1] = map.tokens[i];
                    map.values[i - 1] = map.values[i];
                } else if(!strcmp(map.tokens[i], key)) {
                    found = true;
                    free(map.values[i]);
                }
            }

            if (found) {
                map.count--;
            }
        }
    };
}

#endif