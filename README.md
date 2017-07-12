# ndn-DemoApps
Demo Applications for Named Data Networking

## Requirements:
* ndn-cxx version 0.3.4 (http://named-data.net/)
* nfd	version 0.3.4 (http://named-data.net/)
* libdash 2.2 and up (https://github.com/bitmovin/libdash)

## Commandline-Parameters:
### Producer
| Parameter|Shorthand| Meaning                                                         |
|-----------|---------|-----------------------------------------------------------------|
|prefix|p|Prefix the Producer listens to (Required)|
|document-root|b|The directory open to requests (Interests). (Required)|
|data-size|s |The size of the datapacket in bytes. (Required)|
|freshness-time|f |Freshness time of the content in seconds. (Default 5min)|
|threads| t |Number of threads fullfulling requests (default 2)|

### Dashplayer (Client)
| Parameter|Shorthand| Meaning                                                         |
|-----------|---------|-----------------------------------------------------------------|
|    name|p         |                  The name of the interest to be sent (Required)|
|    adaptionlogic|a|                The name of the adaption-logic to be used (Required)|
|    lifetime|l     |                    The lifetime of the interest in milliseconds (Default 1000ms)|
|   run-time|t      |                 Runtime of the Dashplayer in Seconds. If not specified it is unlimited|
|   request-rate|r  |              Request Rate in kbits assuming 4kb large data packets (Required)|
|   output-file|o   |                Name of the dashplayer trace log file|


## Usage-Example:
"/home/theuerse/data_modified/tmp" contains the MPD (BBB_small.mpd) and the SVC-coded-files.

*sudo nfd-start*


*dashproducer --prefix /Server --document-root /home/theuerse/data_modified/tmp --data-size 4096*
>document-root initialized with: /home/theuerse/data_modified/tmp


*dashplayer --name /Server/BBB_small.mpd -r 2000 -t 20 -l 200 -a buffer -o ./consumer.log*
>Are you deploying this on the PInet? Setting default id = 0
>Fetching MPD: /Server/BBB_small.mpd
>parsing...
>downloading segment = /Server/BBB-I-360p.seg0-L0.svc
>downloading segment = /Server/BBB-I-360p.seg1-L0.svc
>downloading segment = /Server/BBB-I-360p.seg2-L0.svc
>downloading segment = /Server/BBB-I-360p.seg3-L0.svc
>downloading segment = /Server/BBB-I-360p.seg4-L0.svc
>downloading segment = /Server/BBB-I-360p.seg5-L0.svc
>downloading segment = /Server/BBB-I-360p.seg6-L0.svc
>downloading segment = /Server/BBB-I-360p.seg7-L0.svc
>downloading segment = /Server/BBB-I-360p.seg8-L0.svc
>downloading segment = /Server/BBB-I-360p.seg9-L0.svc
>downloading segment = /Server/BBB-I-360p.seg10-L0.svc
>Stop Player!
>Player Finished
>
## Example MPD (BBB_small.mpd)


<?xml version="1.0" encoding="UTF-8"?>
    <MPD xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xmlns="urn:mpeg:DASH:schema:MPD:2011"
         xsi:schemaLocation="urn:mpeg:DASH:schema:MPD:2011"
         profiles="urn:mpeg:dash:profile:isoff-main:2011"
         type="static"
         mediaPresentationDuration="PT20S"
         minBufferTime="PT2.0S">
              <BaseURL>/</BaseURL>
              <Period start="PT0S">
    
     <AdaptationSet bitstreamSwitching="true" mimeType="video/svc" startWithSAP="1" maxWidth="640" maxHeight="360" maxFrameRate="24" par="16:9">
                    <SegmentBase>
                        <Initialization sourceURL="BBB-I-360p.init.svc"/>
                    </SegmentBase>
    
     <Representation id="0" codecs="AVC" mimeType="video/svc"
                                width="640" height="360" frameRate="24" sar="1:1" bandwidth="642933">
                       <SegmentList duration="48" timescale="24">
                     <SegmentURL media="BBB-I-360p.seg0-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg1-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg2-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg3-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg4-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg5-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg6-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg7-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg8-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg9-L0.svc"/>
                         <SegmentURL media="BBB-I-360p.seg10-L0.svc"/>
                       </SegmentList>
                    </Representation>
    
     <Representation id="1" dependencyId="0" codecs="SVC" mimeType="video/svc"
                                width="640" height="360" frameRate="24" sar="1:1" bandwidth="997955">
                       <SegmentList duration="48" timescale="24">
                         <SegmentURL media="BBB-I-360p.seg0-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg1-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg2-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg3-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg4-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg5-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg6-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg7-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg8-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg9-L1.svc"/>
                         <SegmentURL media="BBB-I-360p.seg10-L1.svc"/>
                       </SegmentList>
                    </Representation>
    
     <Representation id="2" dependencyId="0 1" codecs="SVC" mimeType="video/svc"
                                width="640" height="360" frameRate="24" sar="1:1" bandwidth="1405754">
                       <SegmentList duration="48" timescale="24">
                         <SegmentURL media="BBB-I-360p.seg0-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg1-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg2-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg3-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg4-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg5-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg6-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg7-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg8-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg9-L2.svc"/>
                         <SegmentURL media="BBB-I-360p.seg10-L2.svc"/>
                       </SegmentList>
                    </Representation>
    
     </AdaptationSet>
        </Period>
    </MPD>
