# QuidProWiz

QuidProWiz stands for Quidditch Professional Wizard which as the name suggests is a game inspired from the world of Harry Potter where wizards and witches fly on brooms manuvering with a quaffle(ball) to scroe through goals which are three rings. The current implementation only has the player grabbing a quaffle, scoring the quaffle and a bludger to stop the player from scoring.

# Controls
WASD - Broomstick movement controls
E - To pickup quaffle
Left Mouse button - To throw the quaffle
Left Shift - Hold for speed boost

# Technical Design
The design choice I went for based on the conversiton with Vu was to have a data driven approach to make the prototype. I created multiple Data Assets and pushed all the tuning values for each and every aspect I have implemented so far to these data assets making the approach more centralized and easy to mess about especially for a game like quidditch.
The other approach I took was implementing a State Pattern for handling the different states the Quaffle will be in making it more easier to have more states like Steal and Pass in the future.
I have treated this more as the prototype to test the mechanics out and keep implmenting features and polishing the experience in the future.