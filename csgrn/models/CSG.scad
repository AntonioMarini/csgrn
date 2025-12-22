union(){
translate([-24,0,0]) {
    union() {
        cube(15);
        sphere(10);
    }
}

intersection() {
    cube(15);
    sphere(10);
}

translate([24,0,0]) {
    difference() {
        cube(15);
        sphere(10);
    }
}	
}