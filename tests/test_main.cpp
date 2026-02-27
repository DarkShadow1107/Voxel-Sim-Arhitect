/**
 * test_main.cpp  —  Voxel-Sim Architect: Custom Catch2 Entry Point
 *
 * What this file owns:
 *  1. VoxelTestFileLogger   — EventListenerBase that writes a timestamped
 *                             structured log to  <repo>/logs/test_<date>_<time>.log
 *                             on every run. Always active, no flags needed.
 *
 *  2. VoxelConsoleReporter  — Minimal terminal reporter injected as the default
 *                             when the user doesn't pass --reporter explicitly.
 *                             Shows one line per test + a compact summary.
 *                             All verbose detail lives in the log file only.
 *
 *  3. main()  —  Bootstraps a Catch::Session, injects the quiet console
 *                reporter as default, then runs.
 *
 * Compile-time constants (injected by tests/CMakeLists.txt):
 *   VOXEL_SIM_VERSION     — e.g. "Beta-Dev"
 *   VOXEL_TEST_LOGS_DIR   — absolute path to <repo>/logs
 */

// ── Catch2 ───────────────────────────────────────────────────────────────────
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

// ── STL ──────────────────────────────────────────────────────────────────────
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ── Compile-time fallbacks ────────────────────────────────────────────────────
#ifndef VOXEL_SIM_VERSION
#  define VOXEL_SIM_VERSION "unknown"
#endif
#ifndef VOXEL_TEST_LOGS_DIR
#  define VOXEL_TEST_LOGS_DIR "./logs"
#endif

// =============================================================================
//  Shared helpers
// =============================================================================
namespace {

// Full datetime  →  "2026-02-23 14:30:00"
std::string datetimeStr() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// File-safe datetime  →  "2026-02-23_14-30-00"
std::string datetimeFilename() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

std::string trunc(const std::string& s, size_t max = 110) {
    return s.size() <= max ? s : s.substr(0, max - 3) + "...";
}

std::string fmtMs(long long ms) {
    if (ms < 1000) return std::to_string(ms) + " ms";
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << (ms / 1000.0) << " s";
    return ss.str();
}

} // namespace

// =============================================================================
//  1.  FILE LOGGER  (EventListenerBase — always active alongside any reporter)
// =============================================================================
class VoxelTestFileLogger final : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;

    static std::string getDescription() {
        return "Writes a structured timestamped log to <repo>/logs/ on every run";
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    void testRunStarting(Catch::TestRunInfo const&) override {
        m_runStart = std::chrono::steady_clock::now();

        // Ensure logs/ directory exists
        std::string logDir = VOXEL_TEST_LOGS_DIR;
        std::error_code ec;
        fs::create_directories(logDir, ec);

        // Timestamped unique filename
        m_logFilename = logDir + "/test_" + datetimeFilename() + ".log";
        m_log.open(m_logFilename);
        if (!m_log.is_open())                        // fallback: beside exe
            m_log.open("test_" + datetimeFilename() + ".log");

        writeHeader();
    }

    void testCaseStarting(Catch::TestCaseInfo const& info) override {
        m_caseStart   = std::chrono::steady_clock::now();
        m_currentName = info.name;
        m_currentTags = info.tagsAsString();
        m_failDetails.clear();
    }

    void assertionEnded(Catch::AssertionStats const& s) override {
        if (!s.assertionResult.isOk()) {
            const auto& r = s.assertionResult;
            std::ostringstream line;
            line << "    │  ✗  " << trunc(r.getExpressionInMacro())
                 << "\n    │     → " << trunc(r.getExpandedExpression())
                 << "\n    │     @ " << r.getSourceInfo().file
                 << " :" << r.getSourceInfo().line;
            m_failDetails.push_back(line.str());
        }
    }

    void testCaseEnded(Catch::TestCaseStats const& s) override {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_caseStart).count();
        bool ok = s.totals.assertions.failed == 0;

        m_log << (ok ? "  ✓  " : "  ✗  ") << m_currentName << "\n";

        if (!m_currentTags.empty())
            m_log << "    │  tags     : " << m_currentTags << "\n";

        m_log << "    │  assert   : " << s.totals.assertions.passed << " ok";
        if (s.totals.assertions.failed > 0)
            m_log << "  /  " << s.totals.assertions.failed << " FAILED";
        m_log << "    duration : " << fmtMs(ms) << "\n";

        for (const auto& msg : m_failDetails)
            m_log << msg << "\n";

        m_log << "\n";
        if (ok) ++m_casesPassed; else ++m_casesFailed;
    }

    void testRunEnded(Catch::TestRunStats const& s) override {
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_runStart).count();

        auto&  t    = s.totals;
        int    tot  = (int)(t.testCases.passed + t.testCases.failed);
        double rate = tot > 0 ? 100.0 * t.testCases.passed / tot : 100.0;

        m_log
            << "═══════════════════════════════════════════════════════════════\n"
            << "  SUMMARY\n"
            << "═══════════════════════════════════════════════════════════════\n"
            << "  Test cases : " << t.testCases.passed << " passed  /  "
                                 << t.testCases.failed << " failed  /  "
                                 << tot << " total\n"
            << "  Assertions : " << t.assertions.passed << " passed  /  "
                                 << t.assertions.failed << " failed\n"
            << "  Success    : " << std::fixed << std::setprecision(1)
                                 << rate << "%\n"
            << "  Duration   : " << fmtMs(totalMs) << "\n"
            << "  Completed  : " << datetimeStr() << "\n"
            << "  Status     : "
                << (t.assertions.failed == 0
                    ? "ALL TESTS PASSED  ✓\n"
                    : "FAILURES DETECTED  ✗\n")
            << "═══════════════════════════════════════════════════════════════\n";
    }

private:
    void writeHeader() {
        m_log
            << "╔═══════════════════════════════════════════════════════════════╗\n"
            << "║          Voxel-Sim Architect  —  Test Run Log                 ║\n"
            << "╚═══════════════════════════════════════════════════════════════╝\n"
            << "  Version  : " << VOXEL_SIM_VERSION << "\n"
            << "  Date     : " << datetimeStr() << "\n"
            << "  Log file : " << m_logFilename << "\n"
            << "───────────────────────────────────────────────────────────────\n\n";
    }

    std::ofstream            m_log;
    std::string              m_logFilename;
    std::string              m_currentName;
    std::string              m_currentTags;
    std::vector<std::string> m_failDetails;
    int                      m_casesPassed = 0;
    int                      m_casesFailed = 0;
    std::chrono::steady_clock::time_point m_runStart;
    std::chrono::steady_clock::time_point m_caseStart;
};

CATCH_REGISTER_LISTENER(VoxelTestFileLogger)

// =============================================================================
//  2.  MINIMAL CONSOLE REPORTER  (one line per test + compact summary)
//      Default reporter injected in main(). Override with --reporter console
//      (or any other Catch2 reporter) for full verbose output.
// =============================================================================
class VoxelConsoleReporter final : public Catch::StreamingReporterBase {
public:
    using StreamingReporterBase::StreamingReporterBase;

    static std::string getDescription() {
        return "Minimal one-line-per-test console output (Voxel-Sim default)";
    }

    void testRunStarting(Catch::TestRunInfo const&) override {
        m_runStart = std::chrono::steady_clock::now();
        std::cout
            << "\n  Voxel-Sim Architect  v" VOXEL_SIM_VERSION "  — running tests...\n"
            << "  ─────────────────────────────────────────────────────────────\n";
    }

    void testCaseStarting(Catch::TestCaseInfo const& info) override {
        m_caseStart = std::chrono::steady_clock::now();
        m_caseName  = info.name;
    }

    void testCaseEnded(Catch::TestCaseStats const& s) override {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_caseStart).count();
        bool ok = s.totals.assertions.failed == 0;

        std::string name = m_caseName;
        if (name.size() > 56) name = name.substr(0, 53) + "...";

        std::cout << "  " << (ok ? "✓" : "✗") << "  "
                  << std::left << std::setw(58) << name;

        if (!ok)
            std::cout << "  [" << s.totals.assertions.failed << " FAILED]";
        else
            std::cout << "  " << fmtMs(ms);

        std::cout << "\n";
    }

    void testRunEnded(Catch::TestRunStats const& s) override {
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_runStart).count();

        auto&  t    = s.totals;
        int    tot  = (int)(t.testCases.passed + t.testCases.failed);
        double rate = tot > 0 ? 100.0 * t.testCases.passed / tot : 100.0;

        std::cout
            << "  ─────────────────────────────────────────────────────────────\n"
            << "  " << (t.assertions.failed == 0 ? "✓" : "✗")
            << "  " << t.testCases.passed << "/" << tot << " passed"
            << "  (" << std::fixed << std::setprecision(1) << rate << "%)"
            << "  " << fmtMs(totalMs)
            << (t.assertions.failed == 0 ? "  ALL PASSED" : "  FAILURES DETECTED")
            << "\n"
            << "     Full details → " VOXEL_TEST_LOGS_DIR "/test_<date>.log\n\n";
    }

private:
    std::string m_caseName;
    std::chrono::steady_clock::time_point m_runStart;
    std::chrono::steady_clock::time_point m_caseStart;
};

CATCH_REGISTER_REPORTER("voxel-console", VoxelConsoleReporter)

// =============================================================================
//  3.  ENTRY POINT
//      Injects --reporter voxel-console as default when the caller hasn't
//      already specified a reporter, keeping the terminal quiet.
//      Pass --reporter console (or any Catch2 reporter) for full verbose output.
// =============================================================================
int main(int argc, char* argv[]) {
    Catch::Session session;

    bool hasReporter = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--reporter" || arg == "-r" || arg.starts_with("--reporter="))
            hasReporter = true;
    }

    if (!hasReporter) {
        std::vector<const char*> newArgv(argv, argv + argc);
        newArgv.push_back("--reporter");
        newArgv.push_back("voxel-console");
        return session.run((int)newArgv.size(), newArgv.data());
    }

    return session.run(argc, argv);
}

