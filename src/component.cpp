#include "component.h"

Component::Component(IMediator *mediator) : mediator(mediator)
{

}

void Component::set_mediator(IMediator *mediator)
{
	this->mediator = mediator;
}
