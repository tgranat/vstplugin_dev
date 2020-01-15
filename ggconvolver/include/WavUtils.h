// =================================================================================================
// WavUtils.h
//
// Code for reading/writing wav files.
//
// Created by Gerry Beauregard.
// =================================================================================================

#ifndef __WavUtils__
#define __WavUtils__

#include <vector>
#include <string>

using namespace std;

/// Writes an audio file from an interleaved buffer
///
///	@param	path	Path to audio file to write
///	@param	x		Audio data. Will be clipped if outside [-1, 1] range.
/// @param	sr		Sample rate of audio data (e.g. 44100)
/// @param  numCh   Number of channels (e.g. 2 for stereo)
///
void audioWrite(const string& path, const vector<float> &x, int sr, int numCh = 1);

/// Writes an audio file from non-interleaved audio
///
///	@param	path	Path to audio file to write
///	@param	x		Audio data. Will be clipped if outside [-1, 1] range.
/// @param	sr		Sample rate of audio data (e.g. 44100)
///
void audioWrite(const string& path, const vector<vector<float>> &x, int sr);

/// Reads an audio file into interleaved buffer
///
///	@param	path	Path to audio file to read
///	@param	y		Audio data. Full scale is [-1, 1], regardless of the bit-depth.
///	@param	sr		Sample rate (e.g. 44100)
/// @param  numCh   Number of channels (e.g. 2 for stereo)
///
void audioRead(const string& path, vector<float> &x, int &sr, int &numCh);

/// Reads an audio file into non-interleaved buffers
///
///	@param	path	Path to audio file to read
///	@param	y		Audio data. Full scale is [-1, 1], regardless of the bit-depth.
///	@param	sr		Sample rate (e.g. 44100)
///
void audioRead(const string& path, vector<vector<float>> &x, int &sr);


#endif
