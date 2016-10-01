
/*
osu!AutoBot
Copyright (C) 2016  Andrey Tokarev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
#pragma once
#include "vec2f.h"
#include "Split.h" 
#include "TimingPoint.h"
#include "segment.h"
#include <math.h>

using namespace std;

class HitObject
{

public:
	vec2f startPosition;
	int startTime;
	int endTime = 0;
	vector<vec2f> sliderPoints;
	vector<Segment> sliderSegments;
	vec2f PCenter;
	float PRadius;
	float startAng, endAng;

	float PixelLength;
	int sliderTime;
	char sliderType;
	int hitType;
	int stackId = 0;
	bool itSlider() const
	{
		return (hitType & 2) > 0;
	}

	int getStartTime() const //Delay, begining of song.
	{
		return startTime -8;
	}

	int getEndTime() const
	{
		return endTime;// -8;
	}

	int getStack() const
	{
		return stackId;
	}

	vec2f getEndPos()
	{
		float t = 1.0f;
		return getPointByT(t);
	}

	void setStack(int stack)
	{
		stackId = stack;
	}

	int getSliderTime() const
	{
		return sliderTime;
	}

	vec2f getStartPosition() const
	{
		return startPosition;
	}

	vec2f getPointByT(float& t) {
		auto floor = floorf(t);
		t = static_cast<int>(floor) % 2 == 0 ? t - floor : floor + 1.0f - t;
		//if (sliderType == 'P') {
		//	auto dist = PixelLength * t;
		//	auto currDist = 0.0f;
		//	auto oldpoint = startPosition;
		//	auto ct = 0.0f;
		//	while (currDist < dist)
		//	{
		//		auto ang = endAng * ct + startAng * (1.f - ct);
		//		vec2f p{ PCenter.x + PRadius * cosf(ang), PCenter.y + PRadius * sinf(ang) };
		//		currDist += distance(p, oldpoint);
		//		if (currDist > dist) { return oldpoint; }
		//		oldpoint = p;
		//		ct += 1.0f / (PixelLength * 0.5f);
		//	}
		//	return oldpoint;
		//}
		auto dist = PixelLength * t;
		auto currDist = 0.0f;
		auto oldpoint = startPosition;
		///for (int i = 0; i < sliderSegments.size(); i++)
		///{
		///	//auto seg = sliderSegments[i];
		///	//if (i == sliderSegments.size() - 1)
		///	//{
		///	//	auto ct = 0.0f;
		///	//	while (currDist < PixelLength)
		///	//	{
		///	//		//auto p{ bezier(seg.points, ct) };
		///	//		//currDist += distance(p, oldpoint);
		///	//		if (currDist > dist) { return oldpoint; }
		///	//		//oldpoint = p;
		///	//		ct += 1.0f / (seg.points.size() * 50 - 1);
		///	//	}
		///	//}
		///	//for (auto ct = 0.0f; ct < 1.0f + (1.0f / (seg.points.size() * 50 - 1)); ct += 1.0f / (seg.points.size() * 50 - 1))
		///	//{
		///	//	//auto p{ bezier(seg.points, ct) };
		///	//	//currDist += distance(p, oldpoint);
		///	//	//if (currDist > dist) { return oldpoint; }
		///	//	//oldpoint = p;
		///	//}
		///}
		return oldpoint;
	}

	HitObject(string hitstring, vector<TimingPoint> *timingPoints) {
		endTime = 0;
		auto tokens = split_string(hitstring, ",");

		startPosition = vec2f(stof(tokens.at(0)), stof(tokens.at(1)));
		startTime = atoi(tokens.at(2).c_str());
		hitType = atoi(tokens.at(3).c_str());
		if (itSlider())
		{
			auto RepeatCount = atoi(tokens.at(6).c_str());
			PixelLength = stof(tokens.at(7));
			float beatLengthBase = timingPoints->at(0).getBPM();
			float BPM = timingPoints->at(0).getBPM();

			for (auto point : *timingPoints) {
				if (point.getTime() <= startTime) {
					if (point.getBPM() >= 0.0f) {
						beatLengthBase = point.getBPM();
					}
					BPM = point.getBPM();
				}
			}

			if (BPM < 0.0f) { auto newMulti = BPM / -100.0f; BPM = beatLengthBase * newMulti; }

			sliderTime = static_cast<int>(BPM * PixelLength / 100.0f);
			endTime = sliderTime * RepeatCount + startTime;

			auto SliderTokens = split_string(tokens.at(5), "|");

			sliderPoints.push_back(startPosition);

			for (auto i = 1; i < static_cast<int>(SliderTokens.size()); i++) {
				auto p = split_string(SliderTokens.at(i), ":");
				auto point = vec2f(stof(p.at(0)), stof(p.at(1)));
				sliderPoints.push_back(point);
			}

			if (sliderPoints[sliderPoints.size() - 1] == sliderPoints[sliderPoints.size() - 2]) { // EndPoint == red dot
				sliderPoints.resize(sliderPoints.size() - 1);
			}
			sliderType = SliderTokens[0].c_str()[0];
		}
	}

	~HitObject() {

	}
};