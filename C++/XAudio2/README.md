# XAudio2

The BasicSound, BasicStream, CustomAPO, and Sound3D samples were shipped in the DirectX SDK (June 2010). This package inclues the latest versions of each, plus additional XAudio2 samples.

|Sample|Note|
|--|--|
|XAudio2BasicSound|This sample demonstrates the XAudio2 API by showing you how to initialize the XAudio2 engine, create a mastering voice, and play sample files..|
|XAudio2AsyncStream|This sample shows you how to play streaming audio using asynchronous file I/O  and the XAudio2 API.<br /><br />*Originally called "XAudio2BasicStream" in the legacy DirectX SDK.*|
|XAudio2Sound3D|This sample shows you how to use the X3DAudio API with XAudio2 for playing spatialized audio.|
|XAudio2CustomAPO|This sample shows you how to create and use custom APOs with XAudio2.|
|XAudio2Enumerate|This sample shows how to enumerate audio devices and initialize an XAudio2 mastering voice for a specific device.|
|XAudio2MFStream|This samples uses Media Foundation to decode a media audio file (which could be compressed with any number of codecs) and streams it through an XAudio2 voice. This technique is most useful for XAudio 2.8 on Windows 8.x which only supports PCM and ADPCM, and not more aggressive lossy compressed schemes which are supported by Media Foundation.|
|XAudio2WaveBank|This sample shows how to play in-memory audio using an XACT-style wave bank.|
|xwbtool|This command-line tool is a simple way to build XACT-style wave banks without using the legacy DirectX SDK's XACT tool or XACTBLD command-line tool. It can be used to build .xwb files suitable for both the XAudio2AsyncStream or XAudio2WaveBank samples, and are binary compatible with the XACT3 DirectX SDK (June 2010) wave bank format. This tool cannot generate XACT sound banks.|
