#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/identity-management/auth/STSAssumeRoleCredentialsProvider.h>

#include <aws/transcribe/TranscribeServiceClient.h>
#include <aws/transcribe/model/StartTranscriptionJobRequest.h>
#include <aws/transcribe/model/GetTranscriptionJobRequest.h>
#include <aws/transcribe/model/GetTranscriptionJobResult.h>

#include <array>
#include <unistd.h>

#pragma once

class Transcribe
{
public:
    Transcribe();
    ~Transcribe();

    bool AudioPreProcessing(const Aws::String &input_file_path, const Aws::String &output_file_path,
                            float input_start_time, float part_duration, int inter_index);

    Aws::String GetJobLang(const Aws::String &jobName);

    bool CreateTranscribe(const Aws::String &bucketName,
                          const Aws::String &objectName,
                          const Aws::String &jobName);

    //region for s3 bucket:
    bool PutObject(const Aws::String &bucketName,
                               const Aws::String &objectName,
                               const Aws::String &region);

private:

    Aws::Auth::AWSCredentials credentials;
    Aws::SDKOptions options;

    //transcribe settings:
    std::string region = "us-east-2";

    //provide keys to credentials under untrusted platform(MAC_PLATFORM)
    //the cmd can get the keys: aws-export-credentials --profile ... 
    std::string access_key_id = "";
    std::string access_key = "";
    std::string sesson_token = "";

};
