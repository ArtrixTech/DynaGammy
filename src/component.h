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
		GAMMA_SLIDER_MOVED,
		AUTO_BRT_TOGGLED,
		AUTO_TEMP_TOGGLED,
		SYSTEM_WAKE_UP,
		APP_QUIT,
		APP_QUIT_PURE_GAMMA,
	};
};

#endif // COMPONENT_H
