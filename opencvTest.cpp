#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <math.h>
#include <windows.h>
using namespace std;
using namespace cv;
#define jifen 5
#define daodianju 3
#define feichezhanbi 0.73
#define yuanhuanyuzhi 40
#define yuanhuanjingdu 10
#define kuanzeng 90
#define kuanzeng2 70

int shiziji = 0;
int banmasign=0;
Mat imgSour;
struct up {
	Point dian;
	int que;
};

//逆透视图片尺寸
#define CAMERA_H  60
#define CAMERA_W  94
#define uint8 unsigned char
#define OUT_H  60
#define OUT_W  94
uint8 image[CAMERA_H][CAMERA_W];//原图像
uint8 image_final[OUT_H][OUT_W];//逆变换图像
double map_square[CAMERA_H][CAMERA_W][2];//现实映射
int map_int[OUT_H][OUT_W][2];//图像映射

//图片或视频二值化（动态阈值）
Mat er_zhi(Mat imgxiu) {
	Mat imgTemp,imgCvt,imgThr;
	cvtColor(imgxiu, imgCvt, COLOR_BGR2GRAY);
	GaussianBlur(imgCvt, imgTemp, Size(5,5), 0);
	threshold(imgTemp, imgThr, 0, 255, THRESH_BINARY + THRESH_OTSU);
	imshow("二值化", imgThr);
	return imgThr;
}

//画出赛道两侧线和中线
Mat che_dao_xian(Mat imgEr) {
	Mat imgCan, imgDil;
	int shizipan = 0;
	int yuanhuanzuosign=0, yuanhuanyousign = 0;
	vector<Point> yuanhuan;
	int banmapan = 0, banmasign = 0, banmadi[3] = { 0 }, banmashang[3] = { 0 };
	int high = (int)(imgEr.rows*feichezhanbi);
	//获取每一列白色点数据
	vector<int> lieBais(imgEr.cols);
	for (int x = 0; x <imgEr.cols; x++) {
		for (int y = high-1; y >=0; y--) {
			if (imgEr.at<uchar>(y,x) !=0) {
				lieBais[x]++;
			}
			else break;
		}
		if (x > 0 && abs(lieBais[x] - lieBais[x - 1]) >= 60) banmasign++;
		//cout << "列" << x << " " << "白点数" << lieBais[x] << endl;
	}
	if (banmasign >= 6) banmapan = 1;
	//左右寻最长白条
	int maxzuo[2] = {0}, maxyou[2] = {0};
	for (int a = 0; a < lieBais.size(); a++) {
		if (lieBais[a] > maxzuo[1]) {
			maxzuo[0] = a;
			maxzuo[1] = lieBais[a];
		}
	}
	for (int a = lieBais.size()-1; a >=0; a--) {
		if (lieBais[a] > maxyou[1]) {
			maxyou[0] = a;
			maxyou[1] = lieBais[a];
		}
	}
	//cout << "左；"<<maxzuo[0] << " " << maxzuo[1] << " 右；" << maxyou[0] << " " << maxyou[1] << endl;

	//找车道线
	int chushi = 0,chuju=0;
	int shiziZuox = 0,shiziZuoy=0,shiziYoux=0;
	vector<up> zuoDao, youDao,zhongXian;
	for (int y = high - 1; y >=high-maxzuo[1]; y--) {
		//左道
		int zuox = maxzuo[0];
		for (int x = maxzuo[0]; x >= 0; x--) {
			if (imgEr.at<uchar>(y, x) != 0 && imgEr.at<uchar>(y, x - 1) == 0) {
				if (zuoDao.size() >= 1) {
					if (abs(x - (int)zuoDao[zuoDao.size() - 1].dian.x) <= daodianju && abs(y - (int)zuoDao[zuoDao.size() - 1].dian.y) <= daodianju) {
						zuoDao.push_back({ {x, y},0 });
						zuox = x;
						if (banmapan == 1) banmapan = 2;
						break;
					}
				}
				else {
					zuox = x;
					zuoDao.push_back({ {x, y},0 });
					break;
				}
			}
			else if (imgEr.at<uchar>(y, x) != 0 && imgEr.at<uchar>(y, x - 1) != 0 && imgEr.at<uchar>(y, x - 2) != 0) shiziZuox++;
			if (x-3  == 0) {
				break;
			}
		}
		//右道
		int tempx= maxyou[0] - 4;
		for (int x = maxyou[0] - 4; x < imgEr.cols; x++) {         ///////////////////debug////////////
			if (imgEr.at<uchar>(y, x) != 0 && imgEr.at<uchar>(y, x + 1) == 0) {
				if (youDao.size() >= 1) {
					if (abs(x - (int)youDao[youDao.size() - 1].dian.x) <= daodianju && abs(y - (int)youDao[youDao.size() - 1].dian.y) <= daodianju) {
						youDao.push_back({ {x, y },0 });
						tempx = x;
						break;
					}
				}
				else {
					youDao.push_back({ {x, y },0 });
					tempx = x;
					break;
				}
			}
			else if (imgEr.at<uchar>(y, x) != 0 && imgEr.at<uchar>(y, x + 1) != 0 && imgEr.at<uchar>(y, x + 2) != 0) shiziYoux++;
			if (x + 3 == imgEr.cols - 1) {
				break;
			}
		}
		if (imgEr.cols - tempx >= 80) {
			if (banmapan == 2 && imgEr.at<uchar>(y, tempx + 62) != 0 && imgEr.at<uchar>(y, tempx + 63) != 0&& imgEr.rows - maxzuo[1] <= 80) {
				banmadi[0] = zuox;
				banmadi[1] = tempx;
				banmadi[2] = y;
				banmapan = 3;
			}
		}
		if (chuju==0) chuju = tempx - zuox;
		if (abs(shiziZuox - maxzuo[0]) <= 5 &&abs(imgEr.cols - maxyou[0] + 4 - shiziYoux) <= 5) shizipan = 1;
	}
	Point yuanhuanweidian[4];
	//判断圆环存在否，及其位置
	if (abs(zuoDao.size() - youDao.size()) >= yuanhuanyuzhi) {
		int ji1=0,ji2=0,signs=0;
		//左圆环
		if (zuoDao.size() < youDao.size()) {
			int zuominx = 999999;
			for (int n = 0; n < zuoDao.size(); n++) {
				if (zuominx > zuoDao[n].dian.x) zuominx = zuoDao[n].dian.x;
			}
			for (int n = 0; n < youDao.size(); n++) {
				signs = 0;
				for (int x = youDao[n].dian.x; x >(zuominx>5?zuominx-5:1) ; x--) {
					if (imgEr.at<uchar>(youDao[n].dian.y, x) != 0 && imgEr.at<uchar>(youDao[n].dian.y, x - 1) == 0) {
						if (yuanhuanyousign == 1 && ji1 >= 5) {
							yuanhuanyousign = 2;
							yuanhuanweidian[1].x = youDao[n - 5].dian.x - kuanzeng;
							yuanhuanweidian[1].y = youDao[n - 5].dian.y;
							ji1 = 0;
						}
						else if (yuanhuanyousign == 3 && ji1 >= 5) {
							yuanhuanyousign = 4;
							/*yuanhuanweidian[3].x = youDao[n - 5].dian.x - kuanzeng;
							yuanhuanweidian[3].y = youDao[n - 5].dian.y;*/
							break;
						}
						signs = 1;
						ji1++; ji2 = 0;
					}
				}
				if (signs == 0) {
					if (yuanhuanyousign == 0 && ji2 >= 5) {
						yuanhuanyousign = 1;
						yuanhuanweidian[0].x = youDao[n - 5].dian.x-kuanzeng;
						yuanhuanweidian[0].y = youDao[n - 5].dian.y;
						ji2 = 0;
					}
					else if (yuanhuanyousign == 2 && ji2 >= 5) {
						yuanhuanyousign = 3;
						/*yuanhuanweidian[2].x = youDao[n - 5].dian.x - kuanzeng;
						yuanhuanweidian[2].y = youDao[n - 5].dian.y;*/
						ji2 = 0;
					}
					ji2++; ji1 = 0;
				}
				//cout << "ji1: " << ji1 << " ji2: " << ji2 << " "<<yuanhuanyousign<< endl;
			}
		}
		//右圆环
		else {
			int youmaxx = 0;
			for (int n = 0; n < youDao.size(); n++) {
				if (youmaxx < youDao[n].dian.x) youmaxx = youDao[n].dian.x;
			}
			for (int n = 0; n < zuoDao.size(); n++) {
				signs = 0;
				for (int x = zuoDao[n].dian.x; x < youmaxx; x++) {
					if (imgEr.at<uchar>(zuoDao[n].dian.y, x) != 0 && imgEr.at<uchar>(zuoDao[n].dian.y, x + 1) == 0) {
						if (yuanhuanzuosign == 1 && ji1 >= yuanhuanjingdu) {
							yuanhuanzuosign = 2;
							yuanhuanweidian[1].x = zuoDao[n - 5].dian.x + kuanzeng2;
							yuanhuanweidian[1].y = zuoDao[n - 5].dian.y;
							ji1 = 0;
						}
						else if (yuanhuanzuosign == 3 && ji1 >= yuanhuanjingdu) {
							yuanhuanzuosign = 4;
							/*yuanhuanweidian[3].x = zuoDao[n - 5].dian.x + kuanzeng2;
							yuanhuanweidian[3].y = zuoDao[n - 5].dian.y;*/
							break;
						}
						signs = 1;
						ji1++; ji2 = 0;
					}
				}
				if (signs == 0) {
					if (yuanhuanzuosign == 0 && ji2 >= yuanhuanjingdu) {
						yuanhuanzuosign = 1;
						yuanhuanweidian[0].x = zuoDao[n - 5].dian.x + kuanzeng2+10;
						yuanhuanweidian[0].y = zuoDao[n - 5].dian.y;
						ji2 = 0;
					}
					else if (yuanhuanzuosign == 2 && ji2 >= yuanhuanjingdu) {
						yuanhuanzuosign = 3;
						/*yuanhuanweidian[2].x = zuoDao[n - 5].dian.x + kuanzeng2;
						yuanhuanweidian[2].y = zuoDao[n - 5].dian.y;*/
						ji2 = 0;
					}
					ji2++; ji1 = 0;
				}
				//cout << "ji1: " << ji1 << " ji2: " << ji2 << endl;
			}
		}
	}

	//cout << "左道长度" << zuoDao.size() << " 右道长度" << youDao.size() << endl;
	//cout << "圆环左" << yuanhuanzuosign << endl;

	//十字判断显示
	if (shizipan) {
		shiziji++;
	}
	if (shiziji>=3) {
		shiziji = 0;
		putText(imgSour, "CROSS", Point(10, 40), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 0, 255), 2);
	}

	//斑马线判断
	if (banmapan == 3) {
		putText(imgSour, "BANMA_WIRES", Point(10, 40), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 0, 255), 2);
		line(imgSour, Point(banmadi[0], banmadi[2]), Point(banmadi[1], banmadi[2]), Scalar(255, 0, 255), 3.5);
	}

	//圆环判断
	if (yuanhuanzuosign == 4) {
		putText(imgSour, "right circular", Point(10, 40), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 0, 255), 2);
		for (int n = 0; n < 2; n++) {
			line(imgSour, Point(imgEr.cols - 1, yuanhuanweidian[n].y), Point(yuanhuanweidian[n].x, yuanhuanweidian[n].y), Scalar(255, 0, 255), 3.5);
		}
	}else if (yuanhuanyousign == 4) {
		putText(imgSour, "left circular", Point(10, 40), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 0, 255), 2);
		for (int n = 0; n < 2; n++) {
			line(imgSour, Point(0, yuanhuanweidian[n].y), Point(yuanhuanweidian[n].x, yuanhuanweidian[n].y), Scalar(255, 0, 255), 3.5);
		}
	}

	//找中线
	int sizes = zuoDao.size() < youDao.size() ? zuoDao.size() : youDao.size();
	vector<Point> bestZhongxian;
	vector<Point> niheZhongxian;
	//实际中线
	int kai = 0, zhongx = 0, zhongy = 0;
	for (int n = 0; n < sizes; n++) {
		if (zuoDao[n].que == 0 && youDao[n].que == 0) {
			zhongx = (zuoDao[n].dian.x + youDao[n].dian.x) / 2;
			zhongy = (zuoDao[n].dian.y + youDao[n].dian.y) / 2;
			if (kai == 0) {
				zhongXian.push_back({ {zhongx,zhongy},0 });
				kai = 1;
			}
			else if (abs(zhongx - zhongXian[zhongXian.size() - 1].dian.x) <= daodianju && abs(zhongy - zhongXian[zhongXian.size() - 1].dian.y) >=2) {
				zhongXian.push_back({ {zhongx,zhongy},0 });
			}
			//cout << "中线 " << zhongXian[zhongXian.size() - 1].dian.x << " " << zhongXian[zhongXian.size() - 1].dian.y << endl;
		}
	}

	////最小二乘法拟合中线
	//for (int c = 0; c < zhongXian.size(); c += jifen) {
	//	if (c + jifen >= zhongXian.size()) {
	//		break;
	//	}
	//	double aa, bb, xx = 0, yy = 0, xy = 0, xfangs = 0,y_yy,x_xx,xxx_yyy=0,xxx_2=0;
	//	//xy+xy....,xx+xx
	//	for (int n = c; n < c+jifen; n++) {
	//		xy += zhongXian[n].dian.x * zhongXian[n].dian.y;
	//		xfangs += pow(zhongXian[n].dian.x, 2);
	//		xx += zhongXian[n].dian.x;
	//		yy += zhongXian[n].dian.y;
	//	}
	//	xx = xx / jifen;
	//	yy = yy / jifen;
	//	bb = (xy - jifen * xx * yy) / (xfangs - jifen * pow(xx, 2));
	//	aa = yy - bb * xx;
	//	//std::cout << "拟合函数" << aa << " " << bb << endl;
	//	//求出拟合函数对应点
	//	int zuomin = 999999,zuomax=0,youmin=999999, youmax = 0;
	//	for (int n = c; n < c+jifen; n++) {
	//		if (zuomin > zuoDao[n].dian.x) zuomin = zuoDao[n].dian.x;
	//		if (zuomax < zuoDao[n].dian.x) zuomax = zuoDao[n].dian.x;
	//	}
	//	for (int n = c; n < c+jifen; n++) {
	//		if (youmin > youDao[n].dian.x) youmin = youDao[n].dian.x;
	//		if (youmax < youDao[n].dian.x) youmax = youDao[n].dian.x;
	//	}
	//	for (int x = zuomin; x <= youmax; x++) {
	//		int y = bb * x + aa;
	//		if ( y<= zhongXian[c].dian.y && y >= zhongXian[c+jifen].dian.y&& abs(x - (zuomin + youmax) / 2) <= 3) { //3 为误差范围
	//			niheZhongxian.push_back({ x,y });
	//		}
	//	}
	//}
	////拟合中线
	//for (int n = 0; n < niheZhongxian.size(); n++) {
	//	if (niheZhongxian.size() == n + 1) {
	//		break;
	//	}
	//	//circle(imgSour, niheZhongxian[n], 2, Scalar(0, 255, 0), FILLED);
	//	line(imgSour, niheZhongxian[n], niheZhongxian[n + 1], Scalar(0, 255, 0), 4);
	//	std::cout << "拟合中线" << niheZhongxian[n].x << " " << niheZhongxian[n].y << " " << niheZhongxian[n + 1].x << " " << niheZhongxian[n + 1].y << endl;
	//}

	//原始中线
	for (int n = 0; n < zhongXian.size(); n++) {
		if (zhongXian.size() == n + 1) {
			break;
		}
		//circle(imgSour, zhongXian[n].dian, 2, Scalar(0, 255, 0), FILLED);
		line(imgSour, zhongXian[n].dian, zhongXian[n+1].dian, Scalar(0, 255, 0), 4);
		//std::cout << "原始中线" << zhongXian[n].dian.x << " " << zhongXian[n].dian.y << " " << zhongXian[n + 1].dian.x << " " << zhongXian[n + 1].dian.y << endl;
	}

	////画出车道线
	//for (int n = 0; n < sizes; n++) {
	//	circle(imgSour, zuoDao[n].dian, 2, Scalar(255, 0, 0), FILLED);
	//	//cout << "左道" << zuoDao[n].dian.x << " " << zuoDao[n].dian.y << endl;
	//	circle(imgSour, youDao[n].dian, 2, Scalar(0, 0, 255), FILLED);
	//	//cout << "右道" << youDao[n].dian.x << " " << youDao[n].dian.y << endl;
	//}
	// 
	
	for (int n = 0; n < zuoDao.size(); n++) {
		circle(imgSour, zuoDao[n].dian, 2, Scalar(255, 0, 0), FILLED);
	}
	for (int n = 0; n < youDao.size(); n++) {
		circle(imgSour, youDao[n].dian, 2, Scalar(0, 0, 255), FILLED);
	}

	//cout << imgEr.cols << " " << imgEr.rows << endl;
	return imgSour;
}

Mat er_zhi_ni(Mat imgxiu) {
	Mat imgTemp, imgCvt, imgThr;
	cvtColor(imgxiu, imgCvt, COLOR_BGR2GRAY);
	GaussianBlur(imgCvt, imgTemp, Size(5, 5), 0);
	threshold(imgTemp, imgThr, 0, 255, THRESH_BINARY + THRESH_OTSU);
	imshow("二值化", imgThr);
	imgTemp = imgThr;
	resize(imgThr, imgThr, Size(94, 60));
	//cout << imgThr.cols << " " << imgThr.rows << endl;
	for (int y = 0; y < CAMERA_H; y++) {
		for (int x = 0; x < CAMERA_W; x++) {
			image[y][x] = imgThr.at<uchar>(y, x);
		}
	}
	return imgTemp;
}

void ni_tou(void){
	double angle = 0.8;//俯仰角&&<1
	double dep = 3.8;//视点到面距离
	double prop_j = 1;//上下宽度矫正>1
	double prop_i = 0;//密度修正系数>-1&&<1
	double j_large = 1.6;//横向放大倍数
	uint8 i_abodon = 7;//上方舍弃的行数
	double hight = 50;//摄像头高度
	uint8 i;//y轴
	uint8 j;//x轴
	uint8 ii;
	double xg,yg;
	double x0,y0;
	double zt;
	double sin_a,cos_a;
	sin_a = sin(angle);
	cos_a = cos(angle);

	//初始化摄像头坐标系
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][0] = ((float)CAMERA_H / 2 - (float)i + 0.5) / 10;
			map_square[i][j][1] = ((float)j - (float)CAMERA_W / 2 + 0.5) / 10;
		}
	}
	//横向拉伸
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][1] = map_square[i][j][1] * (1 * (CAMERA_H - 1 - i) + (1 / prop_j) * i) / (CAMERA_H - 1);
		}
	}
	//逆透视变换
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			xg = map_square[i][j][1];
			yg = map_square[i][j][0];
			y0 = (yg * dep + hight * cos_a * yg + hight * dep * sin_a) / (dep * cos_a - yg * sin_a);
			zt = -y0 * sin_a - hight * cos_a;
			x0 = xg * (dep - zt) / dep;
			map_square[i][j][1] = x0;
			map_square[i][j][0] = y0;
		}
	}
	double prop_x;//横坐标缩放比例
	prop_x = (OUT_W - 1) / (map_square[i_abodon][CAMERA_W - 1][1] - map_square[i_abodon][0][1]);
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][1] *= prop_x;
			map_square[i][j][1] *= j_large;
			map_square[i][j][1] = map_square[i][j][1] + OUT_W / 2 - 0.5 * OUT_W / CAMERA_W;
		}
	}
	//前后方向
	double move_y;
	double prop_y;
	move_y = map_square[CAMERA_H - 1][0][0];
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][0] -= move_y;
		}
	}
	prop_y = (OUT_H - 1) / map_square[i_abodon][0][0];
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][0] *= prop_y;
			map_square[i][j][0] = OUT_H - OUT_H / CAMERA_H - map_square[i][j][0];
		}
	}
	//前后拉伸
	double dis_ever[CAMERA_H];
	double dis_add[CAMERA_H];
	double adjust_y[CAMERA_H];//每行的调值
	//计算每行宽度
	for (i = 0; i < CAMERA_H; i++)
	{
		dis_ever[i] = ((1 + prop_i) * (CAMERA_H - 1 - i) + (1 - prop_i) * i) / (CAMERA_H - 1);
	}
	dis_add[0] = 0;
	for (i = 0; i < CAMERA_H; i++)
	{
		if (i == 0)
		{
			dis_add[i] = 0;
		}
		else
		{
			dis_add[i] = dis_add[i - 1] + dis_ever[i - 1];
		}
	}
	adjust_y[0] = 1;
	for (i = 1; i < CAMERA_H; i++)
	{
		adjust_y[i] = dis_add[i] / i;
	}

	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][0] *= adjust_y[i];
		}
	}
	double y_fix;
	y_fix = (OUT_H - 1) / map_square[CAMERA_H - 1][0][0];
	for (i = 0; i < CAMERA_H; i++)
	{
		for (j = 0; j < CAMERA_W; j++)
		{
			map_square[i][j][0] *= y_fix;
		}
	}
	//逆映射-前后方向
	double fars;
	double far_min;
	int nears;
	for (i = 0; i < OUT_H; i++)
	{
		far_min = OUT_H;
		for (ii = 0; ii < CAMERA_H; ii++)
		{
			fars = (double)i - (double)(map_square[ii][CAMERA_H / 2][0]);
			if (fars < 0)
			{
				fars = -fars;
			}
			if (fars < far_min)
			{
				far_min = fars;
				nears = ii;
			}
		}
		for (j = 0; j < OUT_W; j++)
		{
			map_int[i][j][0] = nears;
		}

	}

	//左右方向
	int jj;
	double left_lim;
	double right_lim;
	for (i = 0; i < OUT_H; i++){
		//计算每一行取自哪一行
		ii = map_int[i][OUT_W / 2][0];
		left_lim = map_square[ii][0][1];
		right_lim = map_square[ii][CAMERA_W - 1][1];
		for (j = 0; j < OUT_W; j++)
		{
			if (j<left_lim - 1 || j>right_lim + 1)
			{
				map_int[i][j][1] = 255;
			}
			else
			{
				far_min = CAMERA_W;
				for (jj = 0; jj < CAMERA_W; jj++)
				{
					fars = (double)j - (double)(map_square[ii][jj][1]);
					if (fars < 0) fars = -fars;

					if (fars < far_min){
						far_min = fars;
						nears = jj;
					}
				}
				map_int[i][j][1] = nears;
			}
		}
	}
	//逆透视数据写入
	for (int i = 0; i < OUT_H; i++) {
		for (int j = 0; j < OUT_W; j++) {
			if (map_int[i][j][1] == 255) image_final[i][j] = 0x77; //无用位置
			else image_final[i][j] = image[map_int[i][j][0]][map_int[i][j][1]];
		}
	}
}



///题目一 - 二值化

//int main() {
//	int cmin = 0;
//	Mat img, imgEnd, imgTemp;
//
//	//视频模式
//	string path = "data/赛道.mp4";
//	VideoCapture cap(path);
//	while (1) {
//		bool end=cap.read(img);
//		//循环播放
//		if (end == 0) {
//			cap.set(CAP_PROP_POS_FRAMES, 0);
//			cap.read(img);
//		}
//		er_zhi(img);
//		waitKey(10);
//	}
//
//	//图片模式
//	/*string path = "data/f1.jpg";
//	img = imread(path);
//	er_zhi(img);
//	waitKey(0);*/
//
//	return 0;
//}



///题目二和三 - 边线中线、十字、斑马线、圆环

//int main() {
//	string path = "data/圆环.mp4";
//	Mat img,imgchu,imgzhong;
//	VideoCapture cap(path);
//	while (1) {
//		double tickStart = (double)getTickCount();
//		bool wu=cap.read(img);
//		if (wu == 0) {
//			cap.set(CAP_PROP_POS_FRAMES, 0);
//			cap.read(img);
//		}
//		imgSour = img;
//		imgzhong = che_dao_xian(er_zhi(img));
//		double tickEnd = (double)getTickCount();
//		putText(imgzhong, to_string((int)(((tickEnd - tickStart) / (getTickFrequency())) * 1000)), Point(10, 20), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 255, 0), 1.5);
//		putText(imgzhong, "ms", Point(40, 20), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 255, 0), 1.5);
//		imshow("OutPut",imgzhong );
//		waitKey(20);
//	}
//	return 0;
//}



//题目四 - 逆透视

//int main() {
//	string path = "data/赛道.mp4";
//	VideoCapture cap(path);
//	Mat img;
//	while (1) {
//		Mat imgChu(OUT_H, OUT_W, CV_8UC1);
//		bool wu=cap.read(img);
//		if (wu == 0) {
//			cap.set(CAP_PROP_POS_FRAMES, 0);
//			cap.read(img);
//		}
//		//imshow("原视频", img);
//		er_zhi_ni(img);
//		ni_tou();
//		for (int y = 0; y < CAMERA_H; y++) {
//			for (int x = 0; x < CAMERA_W; x++) {
//				imgChu.at<uchar>(y, x) = image_final[y][x];
//			}
//		}
//		resize(imgChu, imgChu, Size(160, 120));
//		imshow("逆透视", imgChu);
//		waitKey(1);
//	}
//	return 0;
//}



//题目五 - 最终

int main() {
	string path = "data/result.mp4";
	Mat img, imgzhong,imger;
	VideoCapture cap(path);
	while (1) {
		Mat imgChu(OUT_H, OUT_W, CV_8UC1);
		double tickStart = (double)getTickCount();
		bool wu = cap.read(img);
		if (wu == 0) {
			cap.set(CAP_PROP_POS_FRAMES, 0);
			cap.read(img);
		}
		imger=er_zhi_ni(img);
		ni_tou();
		for (int y = 0; y < CAMERA_H; y++) {
			for (int x = 0; x < CAMERA_W; x++) {
				imgChu.at<uchar>(y, x) = image_final[y][x];
			}
		}
		imgSour = img;
		imgzhong = che_dao_xian(imger);
		double tickEnd = (double)getTickCount();
		putText(imgzhong, to_string((int)(((tickEnd - tickStart) / (getTickFrequency())) * 1000)), Point(10, 20), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 255, 0), 1.5);
		putText(imgzhong, "ms", Point(40, 20), FONT_HERSHEY_COMPLEX, 0.7, Scalar(0, 255, 0), 1.5);
		resize(imgChu, imgChu, Size(160, 120));
		imshow("逆透视", imgChu);
		imshow("OutPut", imgzhong);
		waitKey(1);
	}
	return 0;
}





