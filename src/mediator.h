/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include "component.h"
#include "defs.h"

class MainWindow;
class GammaCtl;

class IMediator
{
public:
	virtual void notify(Component *sender, Component::Event e) const = 0;
};

class Mediator : public IMediator
{
private:
	GammaCtl   *gammactl;
	MainWindow *wnd;

public:
	Mediator(GammaCtl *c1, MainWindow *c2);
	void notify(Component *sender,  Component::Event event) const override;
};

#endif // CONTROLLER_H
