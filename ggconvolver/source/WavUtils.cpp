// =================================================================================================
// WavUtils.cpp
//
// Code for reading/writing wav files.
//
// Created by Gerry Beauregard.
// =================================================================================================

#include "../include/WavUtils.h"
#include <cstring>
#include <cassert>
#include <fstream>
#include <algorithm>

#ifndef __APPLE__
#include <climits>
#endif
#include <iostream>

using namespace std;

typedef char FOURCC[4];

static const int BITS_PER_SAMPLE = 16; // 16-bits per audio sample

static const int MIN_SAMPLE_RATE = 8000;
static const int MAX_SAMPLE_RATE = 96000;


static void deinterleaveAudio(const vector<float>& interleaved, vector<vector<float>>& split, int numCh)
{
    assert(numCh > 0 && numCh <= 2);

    float* srcPtr = (float*)interleaved.data();
    int n = (int)interleaved.size()/numCh;

    split.resize(numCh);
    for (int ch = 0; ch < numCh; ch++)
        split[ch].resize(n);

    for (int i = 0; i < n; i++)
    {
        for (int ch = 0; ch < numCh; ch++)
            split[ch][i] = *srcPtr++;
    }
}


static void interleaveAudio(const vector<vector<float>>& split, vector<float>& interleaved)
{
    int numCh = (int)split.size();
    assert(numCh > 0 && numCh <= 2);

    int n = (int)split[0].size();

    interleaved.resize(n*numCh);
    float *outP = interleaved.data();
    for (int i = 0; i < n; i++)
    {
        for (int ch = 0; ch < numCh; ch++)
            *outP++ = split[ch][i];
    }
}




void checkProcessorEndianness()
{

	// http://stackoverflow.com/questions/12791864/c-program-to-check-little-vs-big-endian
	// Check whether *processor* is little-endian or big-endian
	// Increasing memory address -->
	//
	// Little-endian: [0x01|0x00|0x00|0x00] Least significant byte is first
	// Big-endian:    [0x00|0x00|0x00|0x01] Least significant byte is last
	int x = 1;
	bool littleEndian = (*(char*)(&x) == 1) ? true : false;
	assert(littleEndian);
}


// Returns true if 4-character codes are equal
bool equalFourCC(const FOURCC a, const FOURCC b)
{
	return (strncmp(a, b, 4) == 0);
}


// Searches from the current file position to find the chunk
// with the specified ID.  The file position initially must be
// on a chunk ID (but not necessarily the specified one). If the
// chunk is found, the file position is just after the chunk size
// (i.e. at the beginning of the chunk's contents) and the chunk
// size is returned. If no matching chunk is found, 0 is returned.

int findChunk(istream &fp, const FOURCC chunkIDToFind)
{
	int count = 0;
	const int MAX_CHUNKS = 100;

	// While we haven't reached the end of the file
	while (count < MAX_CHUNKS && !fp.eof())
	{
		// Get the current position
		long pos = fp.tellg();
		count++;

		// Read chunk ID
		FOURCC chunkID;
		fp.read((char*)&chunkID, sizeof(chunkID));
		if (fp.gcount() != sizeof(chunkID))
			return 0;

		// Read chunk size
		int chunkSize;
		fp.read((char*)&chunkSize, sizeof(chunkSize));
		if (fp.gcount() != sizeof(chunkSize))
			return 0;

		// If the chunk ID matches, return the chunk size
		if (equalFourCC(chunkID, chunkIDToFind))
			return chunkSize;

		// Otherwise, skip this chunk and move to the next
		fp.seekg(pos + chunkSize + 8);
	}

	// Give up! Couldn't find the chunk.
	return 0;
}





// Reads a wav file header
// If successful, return the number of samples

int readWavHeader(
				  istream	&fp,
				  int		&sampleRate,
				  int		&numSamples,
				  short		&numChannels,
				  short		&bitsPerSample )
{
    checkProcessorEndianness();

	//assert( fp != NULL );
	assert( sizeof(int) == 4 );
	assert( sizeof(short) == 2 );

	//	The canonical WAVE format starts with the RIFF header:
	//
	//	0         4   ChunkID          Contains the letters "RIFF" in ASCII form
	//								   (0x52494646 big-endian form).
	FOURCC chunkID;
	fp.read(chunkID, sizeof(chunkID));
	assert(fp.gcount() == sizeof(chunkID));
	assert( equalFourCC(chunkID, "RIFF"));

	//	4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
	//								   4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	//								   This is the size of the rest of the chunk
	//								   following this number.  This is the size of the
	//								   entire file in bytes minus 8 bytes for the
	//								   two fields not included in this count:
	//								   ChunkID and ChunkSize.
	int chunkSize;
	fp.read((char*)&chunkSize, sizeof(chunkSize));
	assert(fp.gcount() == sizeof(chunkSize));

	//	8         4   Format           Contains the letters "WAVE"
	//								   (0x57415645 big-endian form).
	fp.read(chunkID, sizeof(chunkID));
	assert(fp.gcount() == sizeof(chunkID));
	assert(equalFourCC(chunkID, "WAVE"));

	long wavChunkPos = fp.tellg();

	// -- "fmt " subchunk --

	fp.seekg(wavChunkPos);

	int fmtChunkSize = findChunk(fp, "fmt ");
	assert(fmtChunkSize >= 16);

	//	20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
	//								   Values other than 1 indicate some
	//								   form of compression.
	short audioFormat;
	fp.read((char*)&audioFormat, sizeof(audioFormat));
	assert(fp.gcount() == sizeof(audioFormat));
	assert(audioFormat == 1);

	//	22        2   NumChannels      Mono = 1, Stereo = 2, etc.
	fp.read((char*)&numChannels, sizeof(numChannels));
	assert(fp.gcount() == sizeof(numChannels));
	assert(numChannels == 1 || numChannels == 2);

	//	24        4   SampleRate       8000, 44100, etc.
	fp.read((char*)&sampleRate, sizeof(sampleRate));
	assert(fp.gcount() == sizeof(sampleRate));
	assert( sampleRate >= 8000 && sampleRate <= 96000 );

	//	28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
	//int byteRate = sampleRate * numChannels * bitsPerSample/8;
	int byteRate;
	fp.read((char*)&byteRate, sizeof(byteRate));
	assert(fp.gcount() == sizeof(byteRate));

	//	32        2   BlockAlign       == NumChannels * BitsPerSample/8
	//								   The number of bytes for one sample including
	//								   all channels. I wonder what happens when
	//								   this number isn't an integer?
	short blockAlign;
	fp.read((char*)&blockAlign, sizeof(blockAlign));
	assert(fp.gcount() == sizeof(blockAlign));

	//	34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
	//			  2   ExtraParamSize   if PCM, then doesn't exist
	//			  X   ExtraParams      space for extra parameters
	fp.read((char*)&bitsPerSample, sizeof(bitsPerSample));
	assert(fp.gcount() == sizeof(bitsPerSample));
	assert( bitsPerSample % 8 == 0);

	assert( byteRate == sampleRate * numChannels * bitsPerSample/8 );
	assert( blockAlign == numChannels * bitsPerSample/8 );

	// -- "data" subchunk --
	fp.seekg(wavChunkPos);
	int dataChunkSize = findChunk(fp, "data");
	assert(dataChunkSize > 0);

	numSamples = dataChunkSize / blockAlign;

	return numSamples;
}



// Write the wav header
// If successful, return the number of samples
int writeWavHeader(
				   ostream&		fp,
				   const int	sampleRate,
				   const int	numSamples,
				   const short	numChannels,
				   const short	bitsPerSample )
{
    checkProcessorEndianness();

	assert( sampleRate >= 8000 && sampleRate <= 96000 );
	assert( numSamples >= 0 );
	assert( numChannels == 1 || numChannels == 2 );
	assert( bitsPerSample == 8 || bitsPerSample == 16 );
	assert( sizeof(int) == 4 );
	assert( sizeof(short) == 2 );

	//	The canonical WAVE format starts with the RIFF header:
	//
	//	0         4   ChunkID          Contains the letters "RIFF" in ASCII form
	//								   (0x52494646 big-endian form).
	//size_t numWritten;
	fp.write("RIFF", 4);
	//assert(fp.)

	//	4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
	//								   4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	//								   This is the size of the rest of the chunk
	//								   following this number.  This is the size of the
	//								   entire file in bytes minus 8 bytes for the
	//								   two fields not included in this count:
	//								   ChunkID and ChunkSize.
	int subChunk2Size = numSamples*numChannels*bitsPerSample/8;
	int chunkSize = 36 + subChunk2Size;

	fp.write((char*)&chunkSize, sizeof(chunkSize));

	//	8         4   Format           Contains the letters "WAVE"
	//								   (0x57415645 big-endian form).
	fp.write("WAVE", 4);

	//	The "WAVE" format consists of two subchunks: "fmt " and "data":
	//	The "fmt " subchunk describes the sound data's format:
	//
	//	12        4   Subchunk1ID      Contains the letters "fmt "
	//								   (0x666d7420 big-endian form).
	fp.write("fmt ", 4);

	//	16        4   Subchunk1Size    16 for PCM.  This is the size of the
	//								   rest of the Subchunk which follows this number.
	int subChunk1Size = 16;
	fp.write((char*)&subChunk1Size, sizeof(subChunk1Size));

	//	20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
	//								   Values other than 1 indicate some
	//								   form of compression.
	short audioFormat = 1;
	fp.write((char*)&audioFormat, sizeof(audioFormat));

	//	22        2   NumChannels      Mono = 1, Stereo = 2, etc.
	fp.write((char*)&numChannels, sizeof(numChannels));

	//	24        4   SampleRate       8000, 44100, etc.
	fp.write((char*)&sampleRate, sizeof(sampleRate));

	//	28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
	int byteRate = sampleRate * numChannels * bitsPerSample/8;
	fp.write((char*)&byteRate, sizeof(byteRate));

	//	32        2   BlockAlign       == NumChannels * BitsPerSample/8
	//								   The number of bytes for one sample including
	//								   all channels. I wonder what happens when
	//								   this number isn't an integer?
	short blockAlign = numChannels * bitsPerSample/8;
	fp.write((char*)&blockAlign, sizeof(blockAlign));

	//	34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
	//			  2   ExtraParamSize   if PCM, then doesn't exist
	//			  X   ExtraParams      space for extra parameters
	fp.write((char*)&bitsPerSample, sizeof(bitsPerSample));

	//	The "data" subchunk contains the size of the data and the actual sound:
	//
	//	36        4   Subchunk2ID      Contains the letters "data"
	//								   (0x64617461 big-endian form).
	fp.write("data", 4);

	//	40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
	//								   This is the number of bytes in the data.
	//								   You can also think of this as the size
	//								   of the read of the subchunk following this
	//								   number.
	fp.write((char*)&subChunk2Size, sizeof(subChunk2Size));

	return subChunk2Size;

	/*
	 //	44        *   Data             The actual sound data.
	 numWritten = fwrite( data, subChunk2Size, 1, fp );
	 assert( numWritten == 1 );

	 fclose(fp);
	 */
}


// Write an audio file.
void audioWrite(const std::string& path, const std::vector<float> &x, int sr, int numCh)
{
	cout << "1 numCh=" << numCh << endl;
	// Open the output wav file
	ofstream outStream(path.c_str(), ios::out | ios::binary);
	assert(outStream.is_open());

	// Get the number of samples
    int numSamples = (int)x.size()/numCh;
	assert(numSamples > 0);

	// Write the wav header
	writeWavHeader(outStream, sr, numSamples, numCh, BITS_PER_SAMPLE);

	// Convert the samples from normalized [-1.0, 1.0] floating-point to 16-bit shorts
	vector<short> shortBuf(numSamples*numCh);
	float *src = (float*)x.data();
	short *dst = shortBuf.data();
	for (int i = 0; i < numSamples*numCh; i++)
	{
		float flt = *src++;
		flt = std::min<float>(flt, 1.0);
		flt = std::max<float>(flt, -1.0);
		*dst++ = SHRT_MAX * flt;
	}

	// Write the audio data
	outStream.write((char*)shortBuf.data(), numCh*numSamples*sizeof(shortBuf[0]));

	outStream.flush();

	// Close the file
	outStream.close();
}

// Write an audio file starting from split (non-interleaved) data
void audioWrite(const string& path, const vector<vector<float>> &x, int sr)
{
    vector<float> interleaved;
    interleaveAudio(x, interleaved);
    audioWrite(path, interleaved, sr, (int)x.size());
}


// Read an audio file
void audioRead(const std::string& path, std::vector<float> &x, int &sr, int &numCh)
{
	checkProcessorEndianness();

	// Open the input wav file
	ifstream inStream(path.c_str(), ios::in | ios::binary);
	if (!inStream.is_open())
	{
		printf("Couldn't open %s\n", path.c_str());
		assert(inStream.is_open());
	}

	// Read the wav header
	int numSamples;
	short bitsPerSample;
    short numChannels;

	readWavHeader(inStream, sr, numSamples, numChannels, bitsPerSample);
    numCh = numChannels;

	assert(sr >= MIN_SAMPLE_RATE && sr <= MAX_SAMPLE_RATE);
	assert(numCh >= 1);
	assert(bitsPerSample % 8 == 0);

	int bytesPerSample = bitsPerSample / 8;

	// Adjust output vector to correct size to accomodate the samples
	x.resize(numCh*numSamples);

	// Read all the audio data. Since we'll be reading in in multiple formats, we read
	// it as unsigned bytes
	vector<uint8_t> audioData(numChannels*numSamples*bytesPerSample);
	inStream.read((char*)audioData.data(), numCh*numSamples*bytesPerSample);

	// Convert to normalized [-1,1] floating point

	// Get absolute maximum sample value, e.g. for 16-bit audio abs max value is 2^15 = 32768.
	float maxSample = 1 << (bitsPerSample-1);

	// Get scale factor to apply to normalize sum of samples across channels in a single sample frame.
	float scale = 1.0 / maxSample;

	uint8_t *src = audioData.data();
	for (int i = 0; i < numCh*numSamples; i++)
	{
        int32_t sample = 0;
        uint8_t* sampleBytes = (uint8_t*)(&sample); // We assume this is little-endian!

        // Read in the sample value byte-by-byte. Sample is assumed to be stored in
        // little-endian format in the file, so least-significant byte is always first.
        for (int b = 0; b < bytesPerSample; b++)
        {
            sampleBytes[b] = *src++;
        }

        // If it's 1 byte (8 bits) per sample
        if (bytesPerSample == 1)
        {
            // By convention, 8-bit audio is *unsigned* with sample values from 0 to 255. To
            // make it signed, we need to translate 0 to -128.
            sample += CHAR_MIN;
        }
        // Otherwise, if the most-significant bit of most-significant byte is 1, then
        // sample is negative, so we need to set the upper bytes to all 1s.
        else if (sampleBytes[bytesPerSample-1] & 0x80)
        {
            for (size_t b = bytesPerSample; b < sizeof(sample); b++)
            {
                sampleBytes[b] = 0xFF;
            }
        }

		// Apply scale to get normalized mono sample value for this sample frame
		x[i] = scale * sample;
	}

	// Close the file
	inStream.close();
}

// Read into split (not interleaved) buffers
void audioRead(const string& path, vector<vector<float>> &x, int &sr)
{
    int numCh;
    vector<float> interleaved;
    audioRead(path, interleaved, sr, numCh);
    deinterleaveAudio(interleaved, x, numCh);
}
