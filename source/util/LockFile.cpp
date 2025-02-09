// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "LockFile.h"
#include "../logging/LoggerFactory.h"
#include "StringUtils.h"

#include <csignal>
#include <stdexcept>
#include <unistd.h>

using namespace std;
using namespace Aws::Iot::DeviceClient::Util;
using namespace Aws::Iot::DeviceClient::Logging;

constexpr char LockFile::TAG[];
constexpr char LockFile::FILE_NAME[];

LockFile::LockFile(const std::string &filedir, const std::string &process) : dir(filedir)
{
    string fullPath = dir + FILE_NAME;
    ifstream fileIn(fullPath);
    if (fileIn)
    {
        string storedPid;
        if (fileIn >> storedPid && !(kill(stoi(storedPid), 0) == -1 && errno == ESRCH))
        {
            string processPath = "/proc/" + storedPid + "/cmdline";
            string basename = process.substr(process.find_last_of("/\\") + 1);
            string cmdline;
            ifstream cmd(processPath.c_str());

            // check if process contains name
            if (cmd && cmd >> cmdline && cmdline.find(basename) != string::npos)
            {
                LOGM_ERROR(
                    TAG,
                    "Pid %s associated with active process %s in lockfile: %s",
                    Sanitize(storedPid).c_str(),
                    Sanitize(process).c_str(),
                    Sanitize(fullPath).c_str());

                throw runtime_error{"Device Client is already running."};
            }
        }
        // remove stale pid file
        if (remove(fullPath.c_str()))
        {
            LOGM_ERROR(TAG, "Unable to remove stale lockfile: %s", Sanitize(fullPath).c_str());

            throw runtime_error{"Error removing stale lockfile."};
        }
    }
    fileIn.close();

    FILE *file = fopen(fullPath.c_str(), "wx");
    if (!file)
    {
        LOGM_ERROR(TAG, "Unable to open lockfile: %s", Sanitize(fullPath).c_str());

        throw runtime_error{"Can not write to lockfile."};
    }

    string pid = to_string(getpid());
    fputs(pid.c_str(), file);
    fclose(file);
}

LockFile::~LockFile()
{
    string fullPath = dir + FILE_NAME;
    remove(fullPath.c_str());
}
