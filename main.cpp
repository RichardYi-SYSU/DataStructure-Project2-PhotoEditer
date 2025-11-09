#include "CImg.h"
#include <iostream>
#include <vector>
#include <tuple>
#include <fstream>
#include <string>
#include <sstream>
using namespace cimg_library;
using namespace std;

// 定义颜色常量
const unsigned char white[] = {255, 255, 255};
const unsigned char black[] = {0, 0, 0};

// 图像读取与显示的函数
void showImage(const string &filename) {
    CImg<unsigned char> img(filename.c_str());
    img.display(("Display: " + filename).c_str());
}


// 彩色图像转灰度图像的函数
void colorToGray(const string &input, const string &output) {
    CImg<unsigned char> color(input.c_str());
    
    CImg<unsigned char> gray = color.get_RGBtoYCbCr().get_channel(0);
    gray.display("Gray Image");

    // 保存为 CImg 的 ASCII 格式
    gray.save_ascii(output.c_str());

    // 读取刚刚保存的内容
    ifstream fin(output);
    if (!fin) {
        cerr << "无法读取保存的文件：" << output << endl;
        return;
    }

    // 读取第一行
    string header_line;
    getline(fin, header_line);

    // 剩余部分（像素数据）
    string data((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    fin.close();

    // 重新写回并加上标准 P2 头部
    // 这里是因为Cimg库中使用save_ascii码得到的并非标准的ppm格式，需要为其添加文件头部信息，才能让XnViewer读取
    ofstream fout(output);
    fout << "P2\n" << gray.width() << " " << gray.height() << "\n255\n";
    fout << data;
    fout.close();

    cout << "已保存标准 P2 灰度图像：" << output << endl;
}




// 实现图像缩放的函数
void resizeImage(const string &input, const string &output, int newW, int newH) {
    CImg<unsigned char> src(input.c_str());//读取输入图像文件

    //调用Cimg库中的resize函数，使用interpolation_type=1的双线性插值实现缩放
    CImg<unsigned char> resized = src.resize(newW, newH,1);
    resized.display("Resized Image");

    int spectrum = resized.spectrum();  // 通道数，用于区分彩色图像和灰度图像

    //对于彩色和灰度图像，分别执行各自的存储方式
    if (spectrum == 1) {
        // 如果是灰度图像，就存储其ascii码，再修改其文件头部信息，变为标准的ppm格式
        resized.save_ascii(output.c_str());

        // 去掉CImg自定义头并替换为 P2 头
        ifstream fin(output);
        string header_line;
        std::getline(fin, header_line); // 跳过第一行
        string data((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
        fin.close();

        ofstream fout(output);
        fout << "P2\n" << resized.width() << " " << resized.height() << "\n255\n";
        fout << data;
        fout.close();

        cout << "已保存缩放灰度图像 (P2):" << output << endl;
    }
    else if (spectrum == 3) {
        // 如果是彩色图像，由于save_ascii函数会将RGB三个色彩通道分开存储，导致无法得到正常的彩色图像，于是使用手动存储的方法
        ofstream fout(output);
        fout << "P3\n" << resized.width() << " " << resized.height() << "\n255\n";
        
        for (int y = 0; y < resized.height(); y++) {
            for (int x = 0; x < resized.width(); x++) {
                int r = resized(x, y, 0, 0);//表示红色通道
                int g = resized(x, y, 0, 1);//表示绿色通道
                int b = resized(x, y, 0, 2);//表示蓝色通道
                fout << r << " " << g << " " << b << " ";//手动写入RGB值
            }
            fout << "\n";
        }
        fout.close();

        cout << "已保存缩放彩色图像 (P3):" << output << endl;
    }
    else {
        cerr << "非标准通道数：" << spectrum << "，未保存。" << endl;
    }
}



// 实现图像压缩的函数（RLE：行程长度编码压缩算法）
// 将ppm格式的图像压缩为txt格式的文本文件
void compressImage(const string &input, const string &compressedFile) {
    CImg<unsigned char> img(input.c_str());
    int w = img.width(), h = img.height(), c = img.spectrum();

    ofstream fout(compressedFile);
    fout << w << " " << h << " " << c << "\n";//在压缩的txt文件的开头写入图像的宽、高、通道数

    //对灰度图像和彩色图像分别进行RLE压缩
    if (c == 1) {  
        //灰度图像
        //例如：像素值序列为[5,5,5,2,2,3]，则压缩后为[(5,3),(2,2),(3,1)]
        vector<int> vals;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vals.push_back(img(x, y));

        int count = 1;
        for (size_t i = 1; i <= vals.size(); i++) {
            if (i < vals.size() && vals[i] == vals[i - 1]) count++;
            else {
                fout << vals[i - 1] << " " << count << " ";
                count = 1;
            }
        }
    } else if (c >= 3) { 
        //彩色图像
        //由于彩色图像每个像素有三个通道，使用三元组(r,g,b)来表示一个像素值
        //例如：像素值序列为[(255,0,0),(255,0,0),(0,255,0)]，则压缩后为[((255,0,0),2),((0,255,0),1)]
        vector<tuple<int, int, int>> vals;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                vals.push_back({img(x, y, 0, 0), img(x, y, 0, 1), img(x, y, 0, 2)});

        int count = 1;
        for (size_t i = 1; i <= vals.size(); i++) {
            if (i < vals.size() && vals[i] == vals[i - 1]) count++;
            else {
                auto [r, g, b] = vals[i - 1];
                fout << r << " " << g << " " << b << " " << count << " ";
                count = 1;
            }
        }
    }

    fout.close();
    cout << "已完成RLE压缩，结果保存至：" << compressedFile << endl;
}

// 实现图像解压的函数
// 将txt格式的压缩文件解压为ppm格式的图像文件
void decompressImage(const string &compressedFile, const string &output) {
    ifstream fin(compressedFile);
    int w, h, c;
    fin >> w >> h >> c;//读取图像的宽、高、通道数

    if (c == 1) {
        //灰度图像
        vector<int> vals;
        int v, count;
        while (fin >> v >> count)
            for (int i = 0; i < count; i++) vals.push_back(v);//根据RLE压缩格式还原像素值序列

        CImg<unsigned char> img(w, h, 1, 1, 0);
        int idx = 0;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (idx < (int)vals.size()) img(x, y) = vals[idx++];//将还原的像素值填充到图像对象中

        img.save_ascii(output.c_str());//先保存为CImg的ascii码格式，再修改其文件头部信息，变为标准的ppm格式

        // 去掉 CImg 自定义头并替换为 P2 头
        ifstream fin(output);
        string header_line;
        std::getline(fin, header_line); // 跳过第一行
        string data((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
        fin.close();

        ofstream fout(output);
        fout << "P2\n" << img.width() << " " << img.height() << "\n255\n";
        fout << data;
        fout.close();


        img.display("Decompressed Gray");
        cout << "已解压灰度图：" << output << endl;
    } else {
        //彩色图像
        vector<tuple<int, int, int>> vals;
        int r, g, b, count;
        while (fin >> r >> g >> b >> count)
            for (int i = 0; i < count; i++) vals.push_back({r, g, b});//根据RLE压缩格式还原像素值序列

        CImg<unsigned char> img(w, h, 1, 3, 0);
        int idx = 0;
        //将还原的像素值填充到图像对象中
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (idx < (int)vals.size()) {
                    auto [R, G, B] = vals[idx++];
                    img(x, y, 0, 0) = R;
                    img(x, y, 0, 1) = G;
                    img(x, y, 0, 2) = B;
                }

        ofstream fout(output);//手动写入标准的P3格式（ppm格式彩色图）
        fout << "P3\n" << img.width() << " " << img.height() << "\n255\n";
        for (int y = 0; y < img.height(); y++) {
            for (int x = 0; x < img.width(); x++) {
                int r = img(x, y, 0, 0);
                int g = img(x, y, 0, 1);
                int b = img(x, y, 0, 2);
                fout << r << " " << g << " " << b << " ";
            }
            fout << "\n";
        }
        fout.close();

        img.display("Decompressed Color");
        cout << "已解压彩色图：" << output << endl;
    }
    fin.close();
}



// 主菜单
int main() {
    cout << "==============================" << endl;
    cout << "  Project 2 - 简单图像处理程序" << endl;
    cout << "  使用 CImg 库实现" << endl;
    cout << "==============================" << endl;

    while (true) {
        cout << "\n请选择功能：" << endl;
        cout << "1. 读取并显示图像" << endl;
        cout << "2. 彩色图像转灰度" << endl;
        cout << "3. 图像缩放" << endl;
        cout << "4. 图像压缩" << endl;
        cout << "5. 图像解压" << endl;
        cout << "0. 退出程序" << endl;
        cout << "输入选项：";

        int choice;
        cin >> choice;
        if (choice == 0) break;

        string input, output;
        switch (choice) {
        case 1:
            cout << "请输入图像文件名：";
            cin >> input;
            showImage(input);
            break;

        case 2:
            cout << "请输入彩色图像文件名：";
            cin >> input;
            cout << "请输入灰度图输出文件名：";
            cin >> output;
            colorToGray(input, output);
            break;

        case 3:
            int w, h;
            cout << "请输入原图文件名：";
            cin >> input;
            cout << "输入目标宽和高：";
            cin >> w >> h;
            cout << "请输入输出文件名：";
            cin >> output;
            resizeImage(input, output, w, h);
            break;

        case 4:
            cout << "请输入原图文件名：";
            cin >> input;
            cout << "请输入压缩输出文件名(txt)：";
            cin >> output;
            compressImage(input, output);
            break;

        case 5:
            cout << "请输入压缩文件名(txt)：";
            cin >> input;
            cout << "请输入解压输出文件名(ppm)：";
            cin >> output;
            decompressImage(input, output);
            cout << "已完成解压，可通过功能1查看解压后图像。" << endl;
            break;

        default:
                cout << "无效选项，请重新输入！" << endl;
        }
    }

    cout << "程序结束。" << endl;
    return 0;
}
