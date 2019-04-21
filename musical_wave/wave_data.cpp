#include "wave_data.h"
#include <fstream>
#include <cmath>

using std::ifstream;
using std::ofstream;
using std::ios;

wave_head::wave_head() :
    all_size(0),
    format_size(16),
    fmttag(1),
    channel(2),
    samples_per_sec(0),
    bytes_per_sec(0),
    block_align(0),
    bits_per_sample(0),
    data_size(0)
{
    memcpy(riff, "RIFF", 4);
    memcpy(wave, "WAVE", 4);
    memcpy(fmt, "fmt ", 4);
    memcpy(data, "data", 4);
}

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
void wave_head::set_wave_head(int size, int sample_rate, short per_sample)
{
    samples_per_sec = sample_rate;
    bytes_per_sec = sample_rate * channel*per_sample;
    block_align = channel * per_sample;
    bits_per_sample = per_sample * 8;
    data_size = size;
    all_size = data_size + sizeof(wave_head) - 8;
}

musical_data::musical_data() :
    __time_signature__molecular(0),
    __time_signature__denominator(0),
    __rhythm_per_minute(0)
{
    memset(__base_numerical_freq, 0, sizeof(float) * 8);
}

musical_data::musical_data(const char * musical_file_name)
{
    //加载乐谱文件
    load_musical_file(musical_file_name);
}

string musical_data::get_musical_data(int channel, int sample_rate, short bits_per_sample)
{
    string data_buf;
    float first_amplitude = 0;
    int amplitude_data = 0;
    //开始一秒无声
    for (int i = 0; i < sample_rate; ++i)
    {
        for (int j = 0; j < channel; ++j)
        {
            data_buf.append(reinterpret_cast<char*>(&amplitude_data), bits_per_sample);
        }
    }
    int sample_num;//节拍采样次数
    float freq;//节拍频率
    for (auto iter = __rhythm_data_list.begin(); iter != __rhythm_data_list.end(); ++iter)
    {
        sample_num = 60 * sample_rate*iter->rhythm_length_molecular / (iter->rhythm_length_denominator*__rhythm_per_minute); //采集样本总数
        freq = get_rhythm_freq(iter->numerical, iter->tone);
        int i;
        for (i = 0; i < sample_num; ++i)
        {
            if (0 == iter->numerical)
                amplitude_data = 0;
            else
            {
                amplitude_data = static_cast<int>(sin(first_amplitude + 2 * 3.1415926*i / (sample_rate / freq)) * musical_max_amplitude);
                //amplitude_data = get_amplitude_data(first_amplitude, i, sample_rate / freq);
            }
            for (int j = 0; j < channel; ++j)
            {
                data_buf.append(reinterpret_cast<char*>(&amplitude_data), bits_per_sample);
            }
        }
        //纪录下次节拍的起始振幅
        if (0 == iter->numerical)
            first_amplitude = 0;
        else
        {
            first_amplitude = first_amplitude + 2 * 3.1415926*i / (sample_rate / freq);
            //first_amplitude = get_amplitude_data(first_amplitude, i+1, sample_rate / freq);
        }
    }
    amplitude_data = 0;
    //结束一秒无声
    for (int i = 0; i < sample_rate; ++i)
    {
        for (int j = 0; j < channel; ++j)
        {
            data_buf.append(reinterpret_cast<char*>(&amplitude_data), bits_per_sample);
        }
    }
    return data_buf;
}

float musical_data::get_musical_times()
{
    float all_rhythm_count = 0;
    for (auto iter = __rhythm_data_list.begin(); iter != __rhythm_data_list.end(); ++iter)
    {
        all_rhythm_count = all_rhythm_count + (1.0f*iter->rhythm_length_molecular / iter->rhythm_length_denominator);
    }
    return 60.00f*all_rhythm_count / __rhythm_per_minute;
}

bool musical_data::load_musical_file(const char * musical_file_name)
{
    //加载解析乐谱文件
    __rhythm_data_list.clear();
    //读取乐谱文件数据
    ifstream musical_file;
    musical_file.open(musical_file_name, ios::in);
    if (!musical_file.is_open())
        return false;
    char musical_file_data[musical_file_max_size] = {};
    musical_file.read(musical_file_data, sizeof(musical_file_data));
    musical_file.close();
    //解析乐谱
    if (!analysis_musical_file(musical_file_data))
    {
        __rhythm_data_list.clear();
        return false;
    }
    return true;
}

/*
乐谱文件格式示例如下
1=G 4/4 110
|5 6 .1 .2_ 6_|6- 0 0|.3 .2 .3_ .2_ .1_ 5_|6-- 0|
|5 6 .1 .2_ 6_|6- 0 0|.3 .2 .3_ .2_ .1_ 5_|6-- 0|0 0 0 0|
*/
bool musical_data::analysis_musical_file(const char * musical_file_data)
{
    const char* data_ptr = musical_file_data;
    //乐谱文件解析
    __rhythm_data_list.clear();
    char file_data[musical_file_max_line_size];
    //解析乐谱基本信息，获得乐曲的调，节拍速度等，并创建基准音频率表
    if (!(data_ptr = cut_data(data_ptr, file_data, '\n')))
        return false;
    if (!analysis_musical_base_info(file_data))
        return false;

    //解析乐谱节拍
    while ((data_ptr = cut_data(data_ptr, file_data, '|')))
    {
        if (!analysis_musical_rhythm(file_data))
            return false;
    }
    return true;
}

/*
按照分隔符separator分割数据，并用ret_data缓存第一块数据
返回剩余数据起始地址
*/
const char* musical_data::cut_data(const char* musical_file_data, char* ret_data, char separator)
{
    //分割数据
    const char* first_ptr = musical_file_data;
    while (separator == *first_ptr) ++first_ptr;
    const char* last_ptr = first_ptr;
    while (*last_ptr)
    {
        if (separator == *last_ptr)
        {
            break;
        }
        ++last_ptr;
    }
    if (*first_ptr)
    {
        int data_length = last_ptr - first_ptr;
        if (data_length < musical_file_max_line_size)
        {
            memcpy(ret_data, first_ptr, data_length);
            ret_data[data_length] = 0;
            if (*last_ptr)
                return ++last_ptr;
            else
                return last_ptr;
        }
        else return nullptr;
    }
    return nullptr;
}

bool musical_data::analysis_musical_base_info(const char * musical_file_data)
{
    /*
    读取1=G 4/4 110
    G调做1
    4分音符为一拍，每小节4拍
    每分钟110拍
    */
    const char* p = musical_file_data;
    //获得唱名
    int base_numerical = *p++ - '0';
    if (0 >= base_numerical || 7 < base_numerical)
        return false;
    if ('=' != *p++)
        return false;
    //获得唱名对应的音调
    int rising_or_falling_tone = 0;//升调 降调
    char tone;//音调
    while (0 != *p&&' ' != *p)
    {
        if ('#' == *p)
            rising_or_falling_tone++;
        else if ('b' == *p)
            rising_or_falling_tone--;
        else
        {
            if ('A' <= *p&&'G' >= *p)
            {
                tone = *p;
            }
            else
                return  false;
        }
        ++p;
    }
    while (' ' == *p)++p;
    //获取节拍
    __time_signature__molecular = *p++ - '0';
    if (0 >= __time_signature__molecular || 9 <= __time_signature__molecular)
        return false;
    if ('/' != *p++)
        return false;
    __time_signature__denominator = *p++ - '0';
    if (0 >= __time_signature__denominator || 9 <= __time_signature__denominator)
        return false;
    while (' ' == *p)++p;
    //获取节拍速度
    __rhythm_per_minute = atoi(p);

    //创建基准音阶表
    create_base_numerical_freq(base_numerical, tone, rising_or_falling_tone);
    return true;
}

void musical_data::create_base_numerical_freq(int base_numerical, char tone, int rising_or_falling_tone)
{
    /*
    国际标准音 A-la-440HZ为准：
    C 261.6HZ
    D 293.6HZ
    E 329.6HZ
    F 349.2HZ
    G 392HZ
    A 440HZ
    B 493.8HZ
    */
    static float base_numerical_freq[7] =
    {
        440.00f,//A
        493.88f,//B
        261.63f,//C
        293.66f,//D
        329.63f,//E
        349.23f,//F
        392.00f,//G
    };
    float tone_freq = base_numerical_freq[tone - 'A'];
    static int base_numerical_interval[8] = { 0,0,2,4,5,7,9,11 };
    //计算基准唱名频率
    __base_numerical_freq[0] = 0;
    int interval;
    for (int i = 1; i <= 7; ++i)
    {
        interval = base_numerical_interval[i] - base_numerical_interval[base_numerical];
        __base_numerical_freq[i] = tone_freq * pow(2.0f, interval / 12.0f);
    }
}

bool musical_data::analysis_musical_rhythm(const char * musical_rhythm_data)
{
    //解析乐谱节拍
    if ('\n' == *musical_rhythm_data|| '#' == *musical_rhythm_data)
        return true;
    const char* p = musical_rhythm_data;
    char rhythm_buf[musical_file_max_line_size];
    while ((p = cut_data(p, rhythm_buf, ' ')))
    {
        if (!add_musical_rhythm(rhythm_buf))
            return false;
    }
    return true;
}

bool musical_data::add_musical_rhythm(const char * musical_rhythm_data)
{
    const char* p = musical_rhythm_data;
    rhythm_data rhythm = { 0,0,1,1 };
    if ('!' == *p)
    {
        //换气符
        rhythm.rhythm_length_molecular = 1;
        rhythm.rhythm_length_denominator = 16;
        __rhythm_data_list.push_back(rhythm);
        return true;
    }
    while ('.' == *p)
    {
        //高音
        ++rhythm.tone;
        ++p;
    }
    rhythm.numerical = *p++ - '0';
    if (0 > rhythm.numerical || 7 < rhythm.numerical)
        return false;//错误的唱名
    while ('.' == *p)
    {
        //低音
        --rhythm.tone;
        ++p;
    }
    //音阶长度
    while ('_' == *p)
    {
        rhythm.rhythm_length_denominator *= 2;
        ++p;
    }
    while ('-' == *p)
    {
        rhythm.rhythm_length_molecular += rhythm.rhythm_length_denominator;
        ++p;
    }
    __rhythm_data_list.push_back(rhythm);
    return true;
}

float musical_data::get_rhythm_freq(int numerical, int tone)
{
    return __base_numerical_freq[numerical] * pow(2.00f, tone);
}

int musical_data::get_amplitude_data(int first_amplitude, int i, int length)
{
    int m = musical_max_amplitude;
    int l = length;
    /*
    折线公式
    y=4mx/l-m    (0,l/2)
    y=-4mx/l+3m    (l/2,l)
    */
    int first_x = (first_amplitude + m)*l / (4 * m);
    int x = first_x + i;
    if (x >= length)
        x = x - length;
    if (x < l / 2)
        return 4 * m*x / l - m;
    else
        return -4 * m*x / l + 3 * m;
    return 0;
}

wave_data::wave_data(int channel, int sample_rate, short bits_per_sample) :
    __channel(channel),
    __sample_rate(sample_rate),
    __bits_per_sample(bits_per_sample)
{
}

bool wave_data::load_music_file(const char * musical_file_name)
{
    return __musical_data.load_musical_file(musical_file_name);
}

bool wave_data::create_music_wave(const char * wave_file_name)
{
    string music_data = __musical_data.get_musical_data(__channel, __sample_rate, __bits_per_sample);
    __wave_head.channel = __channel;
    __wave_head.set_wave_head(music_data.size(), __sample_rate, __bits_per_sample);
    ofstream wave_file;
    wave_file.open(wave_file_name, ios::out | ios::binary);
    if (!wave_file.is_open())
        return false;
    wave_file.write(reinterpret_cast<char*>(&__wave_head), sizeof(__wave_head));
    wave_file.write(music_data.data(), music_data.size());
    wave_file.close();
    return true;
}
