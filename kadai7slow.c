#include <stdio.h>
#include <ctype.h>
#include <cv.h>
#include <highgui.h>
#include <curses.h>
#include "get_contour.h"

#define PORT "/dev/ttyACM0" //適宜変更のこと
#include "serial2016.h"

#define CAMERA_CENTER_H 90 //カメラサーボの垂直方向中央値（キャリブレーションに利用）
#define CAMERA_CENTER_V 90 //カメラサーボの垂直方向中央値（キャリブレーションに利用）
#define MOTOR_DEFAULT_L 124 //左モータのデフォルト値（キャリブレーションに利用）
#define MOTOR_DEFAULT_R 132 //右モータのデフォルト値（キャリブレーションに利用）
#define CAMERA_INIT_V 70 //カメラサーボの垂直方向初期値
#define CAMERA_INIT_H 100 //カメラサーボの水平方向初期値



void on_mouse(int event, int x, int y, int flags, void *param);

int main(int argc, char **argv)
{
  CvCapture *capture = NULL;
  IplImage *frame;     // キャプチャ画像 (RGB)
  IplImage *frameHSV;   // キャプチャ画像 (HSV)
  IplImage* framePT;    // 透視変換画像 (RGB)
  IplImage* framePTHSV; // 透視変換画像 (HSV)

  IplImage* mask;      // 指定値によるmask (１チャネル)
  IplImage* contour;   // GetLargestContour() の結果
  IplImage** frames[] = {&frame, &frameHSV};
  IplImage** framesPT[] = {&framePT, &framePTHSV};
  contourInfo topContoursInfo[CONTOURS];
  int key;

  init();

  // 実習項目5.0で計測したモーターの中央値をmotor_onに、サーボの中央値をcamera_onにそれぞれセットする
  motor_on(MOTOR_DEFAULT_L, MOTOR_DEFAULT_R); // モーター静止パルス幅のキャリブレーション
  camera_on(CAMERA_CENTER_V,CAMERA_CENTER_H);    // カメラアングルキャリブレーション

  camera_horizontal(CAMERA_INIT_H); // 水平方向のカメラ角度を初期値に
  camera_vertical(CAMERA_INIT_V); // 垂直方向のカメラ角度を初期値に

  // 赤系のHSV色．各自チューニングすること
  uchar minH = 110, maxH = 140;
  uchar minS = 100, maxS = 255;
  uchar minV =  60, maxV = 255;
  CvMat *map_matrix;
  CvPoint2D32f src_pnt[4], dst_pnt[4];

  src_pnt[0] = cvPoint2D32f(181.0, 199.0);
  src_pnt[1] = cvPoint2D32f(110.5, 199.0);
  src_pnt[2] = cvPoint2D32f(104.7, 240.0);
  src_pnt[3] = cvPoint2D32f(184.2, 240.0);
  dst_pnt[0] = cvPoint2D32f(132.5, 240.0);
  dst_pnt[1] = cvPoint2D32f(107.5, 240.0);
  dst_pnt[2] = cvPoint2D32f(107.5, 260.0);
  dst_pnt[3] = cvPoint2D32f(132.5, 260.0);
  map_matrix = cvCreateMat (3, 3, CV_32FC1);
  cvGetPerspectiveTransform (src_pnt, dst_pnt, map_matrix);

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
    capture = cvCaptureFromCAM(argc == 2 ? argv[1][0] - '0' : -1);
  if (capture == NULL) {
    printf("not find camera\n");
    return -1;
  }
  // 解析速度向上のために画像サイズを下げる
  cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_WIDTH, 320);
  cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_HEIGHT, 240);

  frame = cvQueryFrame(capture);
  		frame = cvQueryFrame(capture);
		frame = cvQueryFrame(capture);
		frame = cvQueryFrame(capture);
  frameHSV = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 3);
  framePT = cvCreateImage(cvSize(240,270), IPL_DEPTH_8U, 3);
  framePTHSV = cvCreateImage(cvGetSize(framePT), IPL_DEPTH_8U, 3);
  mask = cvCreateImage(cvGetSize(framePT), IPL_DEPTH_8U, 1);
  contour = cvCreateImage(cvGetSize(framePT), IPL_DEPTH_8U, 3);

  cvNamedWindow("src", CV_WINDOW_AUTOSIZE);
  cvNamedWindow("dst", CV_WINDOW_AUTOSIZE);
  cvNamedWindow("contour", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("src", 60,480);
  cvMoveWindow("dst", 380,480);
  cvMoveWindow("contour", 700,480);
  cvSetMouseCallback("src", on_mouse, (void *)frames);
  cvSetMouseCallback("dst", on_mouse, (void *)framesPT);
  cvSetMouseCallback("contour", on_mouse, (void *)framesPT);


  while (1) {
    frame = cvQueryFrame(capture);
    cvCvtColor(frame, frameHSV, CV_RGB2HSV);
    cvWarpPerspective (frame, framePT, map_matrix,         CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll (100));

    cvCvtColor(framePT, framePTHSV, CV_RGB2HSV);
    cvShowImage("src", frame);
    cvShowImage("dst", framePT);
    GetMaskHSV(framePT, mask, minH, maxH, minS, maxS, minV, maxV);
    GetLargestContour(framePT, mask, contour, topContoursInfo);
    cvShowImage("contour", contour);
    key = cvWaitKey(1);

		int i = 0;

    if(topContoursInfo[0].area > 0)
    {
			printf("見つかったよ\n");
      /// 赤い物体を見つけた場合
      CvBox2D oblique = topContoursInfo[0].oblique; // 認識した物体を囲む長方形
      int height = oblique.size.height;
			int width = oblique.size.width;
      int x = oblique.center.x;                            // 認識した物体の画面内のx座標(0~239)
      int y = oblique.center.y;                            // 認識した物体の画面内のy座標(0~269)

      printf("x : %d ,y : %d  ,height : %d ,width : %d \n", x , y , height, width);

      float angle = oblique.angle * (-1);
      // 垂直二等分線の角度を計算 if 垂直二等分線上に乗っていない場合、、、 (*)までif文の中の処理
        // 回転する角度を計算 もし、さっきのαなら、90-2α
        motor(128-10, 128+10); // 回転、今は一旦左回転
        if (angle <= 45) {
		      printf("angle <= 45  angle : %f \n", angle);
		      usleep((90-2*angle)/23 * 1000000 * 0.7); // 回転角度によって時間を変更
				} else {
          printf("angle > 45  angle : %f \n", angle);
		      usleep((2*angle - 90)/23 * 1000000 * 0.7); 
        }
        motor(156,160);
        usleep((x-118)/23 * 1000000 * 1.5); //時間に距離によって変更
        motor(128+10, 128-10);
        usleep(90/77.9 * 1000000); // 回転角度によって時間を変更
        // (*)

      // この時点で正面に入ってるはず！
      motor(155,160);
      usleep((269-y)/23 * 1000000);

      // 見えなくなってからの処理！
      if (y >= 268) {                                      // y座標で制御
        usleep(0.7*1000000 * 0.8);                               // ()の中に時間(sec)を書く。ここで38.5cm直進をする。 ただこの値は(*)によって変わってくる。
        motor(128,128);                                   // 停止
      }
    }
    else
    {
      /// 赤い物体が見つからなかった場合
      motor(128+10, 128-10); //　その場右回転
      printf("ここのuslep\n");
			// usleep(1 * 650000);
   	}
    ////
    if (key == 'q') break;
  }

  finalize();
  cvDestroyWindow("src");
  cvDestroyWindow("contour");
  cvReleaseImage(&frameHSV);
  cvReleaseImage(&framePTHSV);
  cvReleaseImage(&mask);
  cvReleaseImage(&contour);
  cvReleaseCapture(&capture);
  return 0;
}

void on_mouse(int event, int x, int y, int flags, void *frames)
{
  CvScalar BGR, HSV;
  if (event == CV_EVENT_MOUSEMOVE) {
    IplImage* rgb_image = *(((IplImage***)frames)[0]);
    IplImage* hsv_image = *(((IplImage***)frames)[1]);
    if(y < rgb_image->height && x < rgb_image->width &&
      y < hsv_image->height && x < hsv_image->width)
    {
      BGR = cvGet2D(rgb_image, y, x);
      HSV = cvGet2D(hsv_image, y, x);
      printf("(%3d,%3d): RGB=(%3.0f,%3.0f,%3.0f) HSV=(%3.0f,%3.0f,%3.0f)\n",
             x, y, BGR.val[2], BGR.val[1], BGR.val[0],
             HSV.val[0], HSV.val[1], HSV.val[2]);
    }
  }
}
