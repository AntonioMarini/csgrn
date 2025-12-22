$fn=50;


difference(){

intersection(){
    color("blue",1.0)
        sphere(0.66);
translate([-0.5,-0.5,-0.5]){
    color("red",1.0)
cube(1);
}
}
color("brightgreen",1.0)
union(){

union(){
translate([0,0.0,-1.0]){
    cylinder(2,0.36,0.36);
}

rotate([90,0,0]){
    translate([0,0.0,-1.0]){
        cylinder(2,0.36,0.36);
    }
}
}

rotate([0,90,0]){
    translate([0,0.0,-1.0]){
        cylinder(2,0.36,0.36);
    }
}
}

}
