#include "transcribe.h"

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