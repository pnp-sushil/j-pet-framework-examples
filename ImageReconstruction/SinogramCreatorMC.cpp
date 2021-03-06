/**
 *  @copyright Copyright 2017 The J-PET Framework Authors. All rights reserved.
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may find a copy of the License in the LICENCE file.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  @file SinogramCreatorMC.cpp
 */

#include "SinogramCreatorMC.h"
#include <TH1F.h>
#include <TH2I.h>
using namespace jpet_options_tools;

SinogramCreatorMC::SinogramCreatorMC(const char* name) : SinogramCreator(name) {}

SinogramCreatorMC::~SinogramCreatorMC() {}

bool SinogramCreatorMC::init()
{
  SinogramCreatorMC::setUpOptions();
  fOutputEvents = new JPetTimeWindow("JPetEvent");

  getStatistics().createHistogram(new TH2I("reconstruction_histogram", "reconstruction_histogram histogram",
                                           std::ceil(fMaxReconstructionLayerRadius * 2 * (1.f / fReconstructionDistanceAccuracy)) + 1, 0.f,
                                           fMaxReconstructionLayerRadius, kReconstructionMaxAngle, 0, kReconstructionMaxAngle));

  getStatistics().createHistogram(
      new TH1F("pos_dis", "Position distance real data", (fMaxReconstructionLayerRadius)*10 * 5, 0.f, fMaxReconstructionLayerRadius));
  getStatistics().createHistogram(new TH1F("angle", "Position angle real data", kReconstructionMaxAngle, 0, kReconstructionMaxAngle));

#if ROOT_VERSION_CODE < ROOT_VERSION(6, 0, 0)
  getStatistics().getObject<TH2I>("reconstruction_histogram")->SetBit(TH2::kCanRebin);
  getStatistics().getObject<TH1F>("angle")->SetBit(TH1::kCanRebin);
  getStatistics().getObject<TH1F>("pos_dis")->SetBit(TH1::kCanRebin);
#else
  getStatistics().getObject<TH2I>("reconstruction_histogram")->SetCanExtend(TH1::kAllAxes);
  getStatistics().getObject<TH1F>("angle")->SetCanExtend(TH1::kAllAxes);
  getStatistics().getObject<TH1F>("pos_dis")->SetCanExtend(TH1::kAllAxes);
#endif

  generateSinogram();
  return true;
}

void SinogramCreatorMC::generateSinogram()
{
  float firstX = 0.f;
  float firstY = 0.f;
  float secondX = 0.f;
  float secondY = 0.f;
  float firstZ = 0.f;
  float secondZ = 0.f;
  float firstT = 0.f;
  float secondT = 0.f;
  float skip = 0.f;
  int coincidence = 0;

  int numberOfCorrectHits = 0;
  int totalHits = 1; // to make sure that we do not divide by 0

  if (fSinogram == nullptr)
  {
    fSinogram = new JPetRecoImageTools::Matrix2DProj*[fZSplitNumber];
    for (int i = 0; i < fZSplitNumber; i++)
    { fSinogram[i] = new JPetRecoImageTools::Matrix2DProj(fMaxDistanceNumber, (std::vector<double>(kReconstructionMaxAngle, 0))); } }

  for (const auto& inputPath : fInputData)
  {
    std::ifstream in(inputPath);
    while (in.peek() != EOF)
    {

      in >> firstX >> firstY >> firstZ >> firstT >> secondX >> secondY >> secondZ >> secondT >> skip >> skip >> skip >> skip >> coincidence >> skip >>
          skip >> skip; // read goja data type: https://github.com/JPETTomography/j-pet-gate-tools/tree/master/goja
      // in >> firstX >> firstY >> firstZ >> firstT >> secondX >> secondY >> secondZ >> secondT;

      if (coincidence != 1) // 1 == true event
        continue;

      if (analyzeHits(TVector3(firstX, firstY, firstZ), firstT, TVector3(secondX, secondY, secondZ), secondT))
      {
        numberOfCorrectHits++;
      }
      totalHits++;
    }
  }

  std::cout << "Correct hits: " << numberOfCorrectHits << " total hits: " << totalHits
            << " (correct percentage: " << (((float)numberOfCorrectHits * 100.f) / (float)totalHits) << "%)" << std::endl;
}

bool SinogramCreatorMC::exec() { return true; }

bool SinogramCreatorMC::terminate()
{
  JPetFilterRamLak filter(0.7);
  JPetRecoImageTools::FourierTransformFunction f = JPetRecoImageTools::doFFTW;

  for (int i = 0; i < fZSplitNumber; i++)
  {
    int sliceNumber = i - (fZSplitNumber / 2);
    if (std::find(fReconstructSliceNumbers.begin(), fReconstructSliceNumbers.end(), sliceNumber) == fReconstructSliceNumbers.end())
      continue;
    // save sinogram
    saveResult((*fSinogram[i]), fOutFileName + "sinogram_" + std::to_string(sliceNumber) + "_" + std::to_string(fZSplitRange[i].first) + "_" +
                                    std::to_string(fZSplitRange[i].second) + ".ppm");

    // calculate KDE
    JPetRecoImageTools::Matrix2DProj result = JPetRecoImageTools::backProjectWithKDE((*fSinogram[i]), fTOFInformation[i], (*fSinogram[i])[0].size(),
                                                                                     JPetRecoImageTools::nonRescale, 0, 255);
    // save KDE
    saveResult(result, fOutFileName + "reconstruction_with_KDE_" + std::to_string(sliceNumber) + ".ppm");

    // filter sinogram
    // JPetRecoImageTools::Matrix2DProj filteredSinogram = JPetRecoImageTools::FilterSinogram(f, filter, (*fSinogram[i]));
    // backproject
    // JPetRecoImageTools::Matrix2DProj resultBP =
    //    JPetRecoImageTools::backProject(filteredSinogram, (*fSinogram[i])[0].size(), JPetRecoImageTools::nonRescale, 0, 255);
    // save FBP
    // saveResult(resultBP, fOutFileName + "reconstruction_with_FBP" + std::to_string(sliceNumber) + ".ppm");
  }

  delete[] fSinogram;
  delete[] fMaxValueInSinogram;

  return true;
}

void SinogramCreatorMC::setUpOptions()
{
  auto opts = getOptions();
  if (isOptionSet(opts, kOutFileNameKey))
  {
    fOutFileName = getOptionAsString(opts, kOutFileNameKey);
  }

  if (isOptionSet(opts, kReconstructionDistanceAccuracy))
  {
    fReconstructionDistanceAccuracy = getOptionAsFloat(opts, kReconstructionDistanceAccuracy);
  }
  if (isOptionSet(opts, kZSplitNumber))
  {
    fZSplitNumber = getOptionAsInt(opts, kZSplitNumber);
  }
  if (isOptionSet(opts, kScintillatorLenght))
  {
    fScintillatorLenght = getOptionAsFloat(opts, kScintillatorLenght);
  }
  if (isOptionSet(opts, kMaxReconstructionRadius))
  {
    fMaxReconstructionLayerRadius = getOptionAsFloat(opts, kMaxReconstructionRadius);
  }
  if (isOptionSet(opts, kInputDataKey))
  {
    fInputData = getOptionAsVectorOfStrings(opts, kInputDataKey);
  }
  if (isOptionSet(opts, kEnableObliqueLORRemapping))
  {
    fEnableObliqueLORRemapping = getOptionAsBool(opts, kEnableObliqueLORRemapping);
  }
  if (isOptionSet(opts, kEnableTOFReconstruction))
  {
    fEnableKDEReconstruction = getOptionAsBool(opts, kEnableTOFReconstruction);
  }

  fTOFInformation = new JPetRecoImageTools::Matrix2DTOF[fZSplitNumber];
  fMaxValueInSinogram = new int[fZSplitNumber];
  fCurrentValueInSinogram = new int[fZSplitNumber];
  const float maxZRange = fScintillatorLenght / 2.f;
  float range = (2.f * maxZRange) / fZSplitNumber;
  for (int i = 0; i < fZSplitNumber; i++)
  {
    float rangeStart = (i * range) - maxZRange;
    float rangeEnd = ((i + 1) * range) - maxZRange;
    fZSplitRange.push_back(std::make_pair(rangeStart, rangeEnd));
    fCurrentValueInSinogram[i] = 0;
    fMaxValueInSinogram[i] = 0;
  }

  fMaxDistanceNumber = std::ceil(fMaxReconstructionLayerRadius * 2 * (1.f / fReconstructionDistanceAccuracy)) + 1;

  if (isOptionSet(opts, kReconstructSliceNumbers))
  {
    fReconstructSliceNumbers = boost::any_cast<std::vector<int>>(getOptionValue(opts, kReconstructSliceNumbers));
  }
  else
  {
    for (int i = 0; i < fZSplitNumber; i++) { fReconstructSliceNumbers.push_back(i); }
  }
}
