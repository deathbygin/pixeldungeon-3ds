#include "pd/GLog.h"

namespace pd {

GLog::LogCallback GLog::s_logger = nullptr;

void GLog::setLogger(LogCallback logger) {
    s_logger = std::move(logger);
}

void GLog::i(const std::string& text) {
    if (s_logger && !text.empty()) {
        s_logger(text, COLOR_WHITE);
    }
}

void GLog::p(const std::string& text) {
    if (s_logger && !text.empty()) {
        s_logger(text, COLOR_POSITIVE);
    }
}

void GLog::n(const std::string& text) {
    if (s_logger && !text.empty()) {
        s_logger(text, COLOR_NEGATIVE);
    }
}

void GLog::w(const std::string& text) {
    if (s_logger && !text.empty()) {
        s_logger(text, COLOR_WARNING);
    }
}

void GLog::h(const std::string& text) {
    if (s_logger && !text.empty()) {
        s_logger(text, COLOR_HIGHLIGHT);
    }
}

} // namespace pd

