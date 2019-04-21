#pragma once
#include <list>
#include <string>

using std::list;
using std::string;
//节拍数据结构
class rhythm_data
{
public:
    int numerical;//唱名
    int tone;//音调，0表示中音，1表示高音，2表示倍高音，-1表示低音，-2倍低音
    int rhythm_length_molecular;//节拍长度分子
    int rhythm_length_denominator;//节拍长度分母
};

class musical_data
{
public:
    musical_data();
    musical_data(const char* musical_file_name);
    bool load_musical_file(const char* musical_file_name);
    string get_musical_data(int channel = 2, int sample_rate = 44100, short bits_per_sample = 2);
    float get_musical_times();
private:
    bool analysis_musical_file(const char* musical_file_data);
    const char* cut_data(const char* musical_file_data, char* ret_data, char separator);
    bool analysis_musical_base_info(const char* musical_file_data);
    void create_base_numerical_freq(int base_numerical, char tone, int rising_or_falling_tone);
    bool analysis_musical_rhythm(const char* musical_rhythm_data);
    bool add_musical_rhythm(const char* musical_rhythm_data);
    float get_rhythm_freq(int numerical, int tone);
    int get_amplitude_data(int first_amplitude, int i, int length);

    static const int musical_file_max_size = 1024 * 2;//乐谱文件最大字符数
    static const int musical_file_max_line_size = 128;//乐谱文件每行最大字符数
    static const int musical_max_amplitude = 0x1000;//歌曲节拍最大振幅
    float __base_numerical_freq[8];//基准唱名频率
    int __time_signature__molecular;//拍号分子，分子则表示出每个小节之中有几个单位拍
    int __time_signature__denominator;//拍号分母，分母代表以几分音符为一个单位拍（即单位拍的基本时值）
    int __rhythm_per_minute;//每分钟节拍数
    list<rhythm_data> __rhythm_data_list;//节拍数据结构表
};

class wave_head
{
public:
    char riff[4];//头部分那个"RIFF"
    int all_size;//存的是后面所有文件大小,不包括riff和all_size
    char wave[4];//"WAVE"
    char fmt[4];//"fmt "
    int format_size;//格式长度，在此之前成员的大小,共16个字节
    short fmttag;//音频格式,01为PCM无压缩音频
    short channel;//声道数
    int samples_per_sec;//每秒采样数
    int bytes_per_sec;//每秒数据字节数
    short block_align;//数据块对齐,每次采样字节数据
    short bits_per_sample;//采样位数，采样精度，音频sample的量化位数，有16位，24位和32位等
    char data[4];//"data"
    int data_size;//剩下文件大小，采样数据小端存放
    wave_head();
    /*
    size 音频数据大小
    sample_rate 采样频率，每秒采样次数，主要有22.05KHz，44.1kHz和48KHz。
    per_sample 采样精度

    采样精度说明:
    每个采样数据记录的是振幅, 采样精度取决于储存空间的大小:
    1 字节(也就是8bit) 只能记录 256 个数, 也就是只能将振幅划分成 256 个等级;
    2 字节(也就是16bit) 可以细到 65536 个数, 这已是 CD 标准了;
    4 字节(也就是32bit) 能把振幅细分到 4294967296 个等级, 实在是没必要了.
    */
    void set_wave_head(int size, int sample_rate = 44100, short per_sample = 2);
};

class wave_data
{
public:
    wave_data(int channel = 1, int sample_rate = 44100, short bits_per_sample = 2);
    bool load_music_file(const char* musical_file_name);
    bool create_music_wave(const char* wave_file_name);
private:
    int __channel;
    int __sample_rate;
    short __bits_per_sample;
    wave_head __wave_head;
    musical_data __musical_data;
};