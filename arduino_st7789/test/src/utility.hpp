#pragma once
#include <stdlib.h>
#include <span>

template <typename T>
static size_t file_write_value(FILE* fp, T value) {
    return fwrite(reinterpret_cast<const void*>(&value), sizeof(T), 1, fp);
}

template <typename T>
static size_t file_write_array(FILE* fp, std::span<const T> array) {
    return fwrite(reinterpret_cast<const void*>(array.data()), sizeof(T), array.size(), fp);
}


