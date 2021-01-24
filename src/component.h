/**
 * Copyright (C) Francesco Fusco. All rights reserved.
 * License: https://github.com/Fushko/gammy#license
 */

#ifndef COMPONENT_H
#define COMPONENT_H

class IMediator;

class Component
{
protected:
	IMediator *mediator;
public:
	Component(IMediator *mediator = nullptr);
	void set_mediator(IMediator *mediator);

	enum Event {
		BRT_CHANGED,
		TEMP_CHANGED,
		GAMMA_STEP_CHANGED,
		AUTO_BRT_TOGGLED,
		AUTO_TEMP_TOGGLED,
		SYSTEM_WAKE_UP,
		APP_QUIT,
		APP_QUIT_PURE_GAMMA,
	};
};

#endif // COMPONENT_H
