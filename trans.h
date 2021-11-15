#pragma once

#include <windows.h>
#include "point.h"
#include "size.h"

class trans {
public:
	trans() : c{ 0,0 }, z(1), l{ 0,0 } {}
	// �N���C�A���g�̃T�C�Y
	point p; // �N���C�A���g������W
	size c;
	// �Y�[��
	double z;
	//�@�_�����W�̒����_�B���ꂪ��ʏ�̒����ɕ\��
	point l;
	void l2d(const point* p1, point* p2) const {
		p2->x = p.x + c.w / 2 + (p1->x - l.x) * z;
		p2->y = p.y + c.h / 2 + (p1->y - l.y) * z;
	}
	void d2l(const point* p1, point* p2) const {
		p2->x = l.x + (p1->x - p.x - c.w / 2) / z;
		p2->y = l.y + (p1->y - p.y - c.h / 2) / z;
	}
	void settransfromrect(const point* p1, const point* p2, const int margin) {
		l.x = p1->x + (p2->x - p1->x) / 2;
		l.y = p1->y + (p2->y - p1->y) / 2;
		if (p2->x - p1->x == 0.0 || p2->y - p1->y == 0.0)
			z = 1.0;
		else
			z = min((c.w - margin * 2) / (p2->x - p1->x), (c.h - margin * 2) / (p2->y - p1->y));
	}
};

