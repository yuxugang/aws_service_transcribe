#include "transcribe.h"

using namespace Aws;
using namespace Aws::TranscribeService;
using namespace Aws::TranscribeService::Model;

//#define MAC_PLATFORM  

inline int SystemCommandPipe(const std::string& command, const std::function<void(const char* msg)> &callback)
{
    std::string cmd = command + " 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe)
    {
        std::array<char, 1024> buffer;

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            callback(buffer.data());
    }

    return pclose(pipe);
}

bool Transcribe::AudioPreProcessing(const Aws::String& input_file_path, const Aws::String& output_file_path, \
     float input_start_time, float part_duration,int inter_index)
{

    std::ostringstream ss;
    ss << "ffmpeg -y -hide_banner -nostats";
    ss << " -ss " << input_start_time;
    ss << " -i "  << input_file_path; 
    ss << " -t "  <<  part_duration;
    ss << " -vn";
    ss << " -map "<< "0:a:" << inter_index;
    ss << " -y " << output_file_path; 

    auto cmd = ss.str();
    
    std::string strLayouts;

    auto callback = [&strLayouts](const char *msg) { strLayouts += msg; };

    int ret = SystemCommandPipe(cmd, callback );
    if (ret)
        return false;
    return true; 
}

Transcribe::Transcribe() {

   
   options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;

   Aws::InitAPI(options);

#ifdef MAC_PLATFORM
   credentials.SetAWSAccessKeyId(Aws::String(access_key_id));
   credentials.SetAWSSecretKey(Aws::String(access_key));
   credentials.SetSessionToken(Aws::String(sesson_token));
#endif
}

Transcribe::~Transcribe()
{
    Aws::ShutdownAPI(options);
}
// Test Demo: String job_name = "9ed35b6e-b598-11ec-8e72-acde48001122";
Aws::String Transcribe::GetJobLang(const Aws::String &jobName)
{

    std::ostringstream ss;

    Aws::Client::ClientConfiguration config;

#ifdef MAC_PLATFORM
    TranscribeServiceClient TranscriptClient(credentials, config);
#else
    TranscribeServiceClient TranscriptClient(config);
#endif
    GetTranscriptionJobRequest getTranscriptionJobRequest;
    getTranscriptionJobRequest.SetTranscriptionJobName(jobName);

    auto getTranscriptionJobResult = TranscriptClient.GetTranscriptionJob(getTranscriptionJobRequest);

    if (!getTranscriptionJobResult.IsSuccess())
    {
        std::cout << "Couldn't get job " << jobName << ": "
                  << getTranscriptionJobResult.GetError().GetMessage() << std::endl;
        return "";
    }

    // get back a GetTranscriptionJobResult object.
    const auto &job = getTranscriptionJobResult.GetResult().GetTranscriptionJob();

    TranscriptionJobStatus jb_status = job.GetTranscriptionJobStatus();

    LanguageCode lang_code = job.GetLanguageCode();

    const char *lang_names[] = {"NOT_SET", "af_ZA", "ar_AE", "ar_SA", "cy_GB", "da_DK", "de_CH",
                                "de_DE", "en_AB", "en_AU", "en_GB", "en_IE", "en_IN", "en_US", "en_WL", "es_ES", "es_US", "fa_IR", "fr_CA", "fr_FR",
                                "ga_IE", "gd_GB", "he_IL", "hi_IN", "id_ID", "it_IT", "ja_JP", "ko_KR", "ms_MY", "nl_NL", "pt_BR", "pt_PT",
                                "ru_RU", "ta_IN", "te_IN", "tr_TR", "zh_CN", "zh_TW", "th_TH", "en_ZA", "en_NZ"};

    auto begin = std::chrono::high_resolution_clock::now();
    auto duration = 0;

    while (jb_status != TranscriptionJobStatus::COMPLETED and duration < 30000)
    {

        jb_status = job.GetTranscriptionJobStatus();
        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        sleep(3);
    }

    if (jb_status == TranscriptionJobStatus::COMPLETED)
    {
        lang_code = job.GetLanguageCode();
        ss << lang_names[(int)lang_code];
        return ss.str();
    }
        
    return "";
}

bool Transcribe::CreateTranscribe(const Aws::String &bucketName,
                                  const Aws::String &objectName,
                                  const Aws::String &jobName)
{

    Aws::Client::ClientConfiguration config;

#ifdef MAC_PLATFORM
    TranscribeServiceClient TranscriptClient(credentials, config);
#else
    TranscribeServiceClient TranscriptClient(config);
#endif

    StartTranscriptionJobRequest startTranscriptionJobRequest;
    startTranscriptionJobRequest.SetTranscriptionJobName(jobName);
    startTranscriptionJobRequest.SetIdentifyLanguage(true);
    // Pre-Encoder will transfer audio file to mp4 foramt:
    startTranscriptionJobRequest.SetMediaFormat(MediaFormat::mp4);

    Media media;
    String mediaURI;
    mediaURI = "s3://" + bucketName + "/" + objectName;
    media.SetMediaFileUri(mediaURI);
    startTranscriptionJobRequest.SetMedia(media);

    TranscriptClient.StartTranscriptionJob(startTranscriptionJobRequest);

    GetTranscriptionJobRequest *request = new GetTranscriptionJobRequest();
    request->WithTranscriptionJobName(jobName);
    GetTranscriptionJobOutcome result = TranscriptClient.GetTranscriptionJob(*request);

    if (result.IsSuccess())
        return true;
    return false;
}

// snippet-start:[s3.cpp.put_object.code]
bool Transcribe::PutObject(const Aws::String& bucketName, 
    const Aws::String& objectName,
    const Aws::String& region)
{
    // Verify that the file exists.
    struct stat buffer;

    if (stat(objectName.c_str(), &buffer) == -1)
    {
        std::cout << "Error: PutObject: File '" <<
            objectName << "' does not exist." << std::endl;

        return false;
    }

    Aws::Client::ClientConfiguration config;

    if (!region.empty())
    {
        config.region = region;
    }

#ifdef MAC_PLATFORM
    Aws::S3::S3Client s3_client(credentials, config);
#else
    Aws::S3::S3Client s3_client(config);
#endif

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    //We are using the name of the file as the key for the object in the bucket.
    //However, this is just a string and can set according to your retrieval needs.
    request.SetKey(objectName);

    std::shared_ptr<Aws::IOStream> input_data = 
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag", 
            objectName.c_str(), 
            std::ios_base::in | std::ios_base::binary);

    request.SetBody(input_data);

    Aws::S3::Model::PutObjectOutcome outcome = 
        s3_client.PutObject(request);

    if (outcome.IsSuccess()) {

        std::cout << "Added object '" << objectName << "' to bucket '"
            << bucketName << "'.";
        return true;
    }
    else 
    {
        std::cout << "Error: PutObject: " << 
            outcome.GetError().GetMessage() << std::endl;
       
        return false;
    }
}

#if 0
int main()
{
    Transcribe trans;

    //step 1: Get one clip file:
    trans.AudioPreProcessing("audio_4_channels.mov", "audio_4_channels.mp4", 10, 30, 2);

    //step 2: Upload the file to S3:
    const Aws::String bucket_name = "tubi-video-team";
    const Aws::String object_name = "audio_4_channels.mp4";
    const Aws::String region = "us-east-2";
    if (!trans.PutObject(bucket_name, object_name, region))
    {
        return 1;
    }

    //step 3: Create trans job:
    trans.CreateTranscribe("tubi-video-team", "4_spa_1a3511e0d3214db79bd9fc3e0761ffa6.mp4","test-job-4-spa");
    
    //step 4: Get language:
    trans.GetJobLang("test-job-4-spa");

    return 0;
}
# endif