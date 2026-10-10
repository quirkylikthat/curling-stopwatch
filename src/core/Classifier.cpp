#include "Classifier.h"

namespace curl {

const Band* classify(const Band* table, size_t count, float seconds) {
    if (!table || count == 0) return nullptr;
    for (size_t i = 0; i < count; ++i) {
        if (seconds < table[i].maxSeconds) return &table[i];
    }
    return &table[count - 1];
}

}  // namespace curl
