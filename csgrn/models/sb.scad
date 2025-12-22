difference(){

intersection(){
        sphere(0.66);
translate([-0.5,-0.5,-0.5]){
cube(1);
}
}
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
color([1,0,0])
rotate([0,90,0]){
    translate([0,0.0,-1.0]){
        cylinder(2,0.36,0.36);
    }
}
}

}