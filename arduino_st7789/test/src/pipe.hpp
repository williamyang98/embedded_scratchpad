#pragma once

#include <stdint.h>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <span>

class Pipe {
private:
    bool m_is_closed = false;
    size_t m_write_index = 0;
    size_t m_read_index = 0;
    size_t m_total_data_bytes = 0;
    std::vector<uint8_t> m_buffer;
    std::mutex m_mutex;
    std::condition_variable m_read_cv;
    std::condition_variable m_write_cv;
public:
    Pipe(size_t size) {
        m_buffer.resize(size);
    }
    ~Pipe() {
        close();
    }
    bool close() {
        auto lock = std::unique_lock(m_mutex);
        if (m_is_closed) return false;

        m_is_closed = true;
        m_read_cv.notify_all();
        m_write_cv.notify_all();
        return true;
    }
    int read() {
        auto lock = std::unique_lock(m_mutex);
        const auto wait_condition = [this]() {
            return (m_total_data_bytes > 0) || m_is_closed;
        };
        while (true) {
            if (wait_condition()) break;
            m_read_cv.wait(lock, wait_condition);
        }
        if (m_total_data_bytes == 0 && m_is_closed) return -1;
        const uint8_t data = m_buffer[m_read_index];
        m_read_index++;
        if (m_read_index >= m_buffer.size()) {
            m_read_index = 0;
        }
        m_total_data_bytes--;
        m_write_cv.notify_all();
        return static_cast<int>(data);
    }
    size_t read_block(std::span<uint8_t> data) {
        auto lock = std::unique_lock(m_mutex);
        const auto wait_condition = [this]() {
            return (m_total_data_bytes > 0) || m_is_closed;
        };

        size_t total_read = 0;
        while (data.size() > 0) {
            while (true) {
                if (wait_condition()) break;
                m_read_cv.wait(lock, wait_condition);
            }
            if (m_total_data_bytes == 0 && m_is_closed) break;
            const size_t total_to_read = m_total_data_bytes >= data.size() ? data.size() : m_total_data_bytes;
            for (size_t i = 0; i < total_to_read; i++) {
                data[i] = m_buffer[m_read_index];
                m_read_index++;
                if (m_read_index >= m_buffer.size()) {
                    m_read_index = 0;
                }
            }
            m_total_data_bytes -= total_to_read;
            total_read += total_to_read;
            data = data.subspan(total_to_read);
            m_write_cv.notify_all();
        }
        return total_read;
    }
    size_t write(const uint8_t data) {
        auto lock = std::unique_lock(m_mutex);
        const auto wait_condition = [this]() {
            return (m_total_data_bytes < m_buffer.size()) || m_is_closed;
        };
        while (true) {
            if (wait_condition()) break;
            m_write_cv.wait(lock, wait_condition);
        }
        if (m_is_closed) return 0;
        m_buffer[m_write_index] = data;
        m_write_index++;
        if (m_write_index >= m_buffer.size()) {
            m_write_index = 0;
        }
        m_total_data_bytes++;
        m_read_cv.notify_all();
        return 1;
    }
    size_t write_block(std::span<const uint8_t> data) {
        auto lock = std::unique_lock(m_mutex);
        size_t total_written = 0;
        const auto wait_condition = [this]() {
            return (m_total_data_bytes < m_buffer.size()) || m_is_closed;
        };
        while (data.size() > 0) {
            while (true) {
                if (wait_condition()) break;
                m_write_cv.wait(lock, wait_condition);
            }
            if (m_is_closed) break;
            const size_t total_space_available = m_buffer.size()-m_total_data_bytes;
            const size_t total_to_write = total_space_available >= data.size() ? data.size() : total_space_available;
            for (size_t i = 0; i < total_to_write; i++) {
                m_buffer[m_write_index] = data[i];
                m_write_index++;
                if (m_write_index >= m_buffer.size()) {
                    m_write_index = 0;
                }
            }
            m_total_data_bytes += total_to_write;
            total_written += total_to_write;
            data = data.subspan(total_to_write);
            m_read_cv.notify_all();
        }
        return total_written;
    }
};
