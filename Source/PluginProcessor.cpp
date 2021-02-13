/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include "util/buffer/ring/RingBuffer.h"

//==============================================================================
RadioFilterAudioProcessor::RadioFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                        ),
#else
    :
#endif
//�����o�ϐ�������
paramGain(nullptr),
paramFreqMin(nullptr),
paramFreqMax(nullptr),
rbInL(nullptr),
rbInR(nullptr),
rbTmpL(nullptr),
rbTmpR(nullptr),
rbOutL(nullptr),
rbOutR(nullptr),
window(),
fft(fftOrder)
{
    /* �p�����[�^������ (���|�C���^�[�̏��L���͐e�N���X�ɂ���ĊǗ������̂Ŏ蓮�ł̉���͕s�v) */
    //�Q�C����
    addParameter(paramGain =
        new juce::AudioParameterFloat(
            "gain", //�p�����[�^ID
            "Gain", //�p�����[�^��
            0.0f,   //�ŏ��l
            1.0f,   //�ő�l
            1.0f    //�����l
        ));
    //�Q�C����
    addParameter(paramFreqMin =
        new juce::AudioParameterFloat(
            "freqMin",      //�p�����[�^ID
            "FreqMin",      //�p�����[�^��
            1.0f,           //�ŏ��l
            4000.0f,        //�ő�l
            650.0f          //�����l
        ));
    //�Q�C����
    addParameter(paramFreqMax =
        new juce::AudioParameterFloat(
            "freqMax",      //�p�����[�^ID
            "FreqMax",      //�p�����[�^��
            1.0f,           //�ŏ��l
            4000.0f,        //�ő�l
            2500.0f         //�����l
        ));
}

RadioFilterAudioProcessor::~RadioFilterAudioProcessor()
{
}

//==============================================================================
const juce::String RadioFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RadioFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RadioFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RadioFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RadioFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RadioFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RadioFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RadioFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RadioFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void RadioFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RadioFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /* ������ */
    //�T�C�Y�v�Z (�Ƃ肠�����Œ�K�v�T�C�Y�̔{�m�ۂ��Ă���)
    const unsigned int bufferSize = fftSize < samplesPerBlock ? samplesPerBlock * 2 : fftSize * 2;

    //�����O�o�b�t�@������ (���o�͂��X�e���I�ŏ��������Ă���̂�2�`�����l��)
    rbInL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbInR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbTmpL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbTmpR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbOutL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbOutR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);

    //���֐��p�z�񏉊���
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), fftSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hamming);
}

void RadioFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RadioFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RadioFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //����
    auto* inL = buffer.getReadPointer(0);
    auto* inR = buffer.getReadPointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        rbInL->Write(inL[sample]);
        rbInR->Write(inR[sample]);
    }

    //�G�t�F�N�g����
    processEffect(rbInL, rbTmpL, rbOutL);
    processEffect(rbInR, rbTmpR, rbOutR);

    //�o��
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    processOutSamples(rbOutL, outL, buffer.getNumSamples());
    processOutSamples(rbOutR, outR, buffer.getNumSamples());
}

//==============================================================================
bool RadioFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RadioFilterAudioProcessor::createEditor()
{
    //����ŕ`�揈���͎��O�ōs��Ȃ��̂Ńf�t�H���g�̂��̂��g��
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void RadioFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //�p�����[�^�̕ۑ�
    std::unique_ptr <juce::XmlElement> xml(new juce::XmlElement("AlstiaRadioFilter"));
    xml->setAttribute("gain", (double)*paramGain);
    xml->setAttribute("freqMin", (double)*paramFreqMin);
    xml->setAttribute("freqMax", (double)*paramFreqMax);
    copyXmlToBinary(*xml, destData);
}

void RadioFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //�p�����[�^�̓ǂݍ���
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName("AlstiaRadioFilter")) {
            *paramGain = (float)xmlState->getDoubleAttribute("gain", 1.0);
            *paramFreqMin = (float)xmlState->getDoubleAttribute("freqMin", 650.0);
            *paramFreqMax = (float)xmlState->getDoubleAttribute("freqMax", 2500.0);
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RadioFilterAudioProcessor();
}



void RadioFilterAudioProcessor::processEffect(
    std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf,
    std::shared_ptr<Alstia::Buffer::RingBuffer>& tmpBuf,
    std::shared_ptr<Alstia::Buffer::RingBuffer>& outBuf)
{
    //�V�t�g�ʌv�Z
    const int fftShift = (int)(fftSize * overlap);

    //�T���v�����[�g�擾(44100.000...)
    const double sampleRate = getSampleRate();

    //FFT���ăG�t�F�N�g��������
    while (true) {
        //�K�v�f�[�^�ʁA�o�̓o�b�t�@���`�F�b�N
        if (inBuf->GetDataSize() < fftSize || (outBuf->GetBufferSize() - outBuf->GetDataSize()) < fftSize) break;

        /* �G�t�F�N�g���� */
        //�����p�o�b�t�@�m��
        std::array<float, fftSize * 2> fftBuff;
        //�R�s�[
        for (int i = 0; i < fftSize; i++) {
            fftBuff[i] = inBuf->Read(i);
        }
        //�ǂݎ���������̃f�[�^���o�b�t�@�������
        inBuf->Erase(fftShift);


        /* ���֐���K�p(�n�~���O��) */
        juce::FloatVectorOperations::multiply(fftBuff.data(), window.data(), fftSize);


        /* FFT���s */
        fft.performRealOnlyForwardTransform(fftBuff.data());


        /* ���g���t�B���^���s */
        if (paramFreqMin->get() < paramFreqMax->get()) {
            for (int i = 0; i < fftSize; i++) {
                //Hz
                double hz = sampleRate / fftSize * i;

                if (!(hz >= paramFreqMin->get() && hz <= paramFreqMax->get())) {
                    fftBuff[i] = 0.0f;
                }
            }
        }


        /* IFFT���s */
        fft.performRealOnlyInverseTransform(fftBuff.data());



        /* �o�b�t�@�ɏ����o�� */
        //�I�[�o�[���b�v���������ďo�̓o�b�t�@�ɏ����o�� (���t���[���̑O���f�[�^+�O�t���[���̌㔼�f�[�^)
        for (unsigned int i = 0; i < fftShift; i++) {
            outBuf->Write((fftBuff[i] + tmpBuf->Read(i)) * paramGain->get());
        }

        //���t���[���Ŏg�p�����I�[�o�[���b�v�p�̃f�[�^���폜
        tmpBuf->Erase(fftShift);

        //�ꎞ�o�b�t�@�Ɍ��t���[���̌㔼�f�[�^���i�[
        for (unsigned int i = 0; i < fftShift; i++) {
            tmpBuf->Write(fftBuff[fftSize - fftShift + i]);
        }

        /* �I�[�o�[���b�v�Q�l����
         * https://watlab-blog.com/2019/04/17/python-overlap/
         * http://www.kumikomi.net/archives/2010/09/ep30rir2.php?page=4
         */
    }
}

void RadioFilterAudioProcessor::processOutSamples(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, float* outBuf, const int numSamples)
{
    if (inBuf->GetDataSize() >= numSamples) {
        //�o�b�t�@���R�s�[
        for (int i = 0; i < numSamples; i++) {
            outBuf[i] = inBuf->Read(i);
        }
        //�o�b�t�@��������ɂ���
        inBuf->Erase(numSamples);
    }
    else {
        //0����
        for (int i = 0; i < numSamples; i++) {
            outBuf[i] = 0;
        }
    }
}
