////////////////////////////////////////////////////////////////////////////////
//
// Ryan Singh ma001rs@gold.ac.uk
//
// L-Systems project for Mathematics and Graphics for Games
//

#include <fstream>

//NEED TO CHANGE ALL OF THIS
namespace octet {
  class lsystem : public app {

    ref<visual_scene> app_scene;

    struct branch{
      mat4t line;
      mat4t leaf;
    };

    //generation of the l-system rules
    dynarray<std::string> rules;
    std::string axiom;
    std::string current_iteration;
    color_shader line_shader;
    float angle = 0.0f;
    int max_iterations, iterations = 0;

    //generation of the lines
    dynarray<branch> branches;
    dynarray<mat4t>modelToWorld_stack;
    mat4t modelToWorld;
    vec4 line_colour = vec4(0.2f, 1.0f, 0.0f, 1.0f);
    vec4 leaf_colour = vec4(0.3f, 0.0f, 1.0f, 1.0f);
    bool generate_leaves = false;
    float line_height = 2.0f;
    float line_width = 1.0f;
    float line_w_max = 3.0f;



  public:
    /// this is called when we construct the class before everything is initialised.
    lsystem(int argc, char **argv) : app(argc, argv) {
    }

    void draw_tree(){
      //go through every line in the line array and render them
      for (unsigned i = 0; i < branches.size(); ++i){
        mat4t modelToProjection = mat4t::build_projection_matrix(branches[i].line, app_scene->get_camera_instance(0)->get_node()->access_nodeToParent());
        line_shader.render(modelToProjection, line_colour);

        //Source: http://antongerdelan.net/opengl/vertexbuffers.html - Accessed 4/12/2014
        float points[6] = { 0.0f, 0.0f, 0.0f, 0.0f, line_height, 0.0f }; //just going to cast this for the GLvoid pointer

        glLineWidth(line_width); //width of line (default is 1.0f)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)points); //our array of generic vertex data
        glEnableVertexAttribArray(0); //has to be enabled, disabled by default

        glDrawArrays(GL_LINES, 0, 2); //draw lines
        //Source end

        if (generate_leaves){
          mat4t modelToProjection = mat4t::build_projection_matrix(branches[i].leaf, app_scene->get_camera_instance(0)->get_node()->access_nodeToParent());
          line_shader.render(modelToProjection, leaf_colour);

          static const float vertex_data[] = {
            0.0f, 0.0f, 0.0f,
            0.35f, 0.4f, 0.0f,
            0.2f, 0.3f, 0.0f,
            0.25f, 0.75f, 0.0f,
            0.1f, 0.6f, 0.0f,
            0.0f, 0.8f, 0.0f,
            -0.1f, 0.6f, 0.0f,
            -0.25f, 0.75f, 0.0f,
            -0.2f, 0.3f, 0.0f,
            -0.35f, 0.4f, 0.0f,
            0.0f, 0.0f, 0.0f,
          };

          glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)vertex_data); //our array of generic vertex data
          glEnableVertexAttribArray(0); //has to be enabled, disabled by default

          glDrawArrays(GL_TRIANGLE_FAN, 0, 11); //draw lines
        }
      }
    }

    void forward(){
      branch l;
      l.line = modelToWorld;
      branches.push_back(l);
      l.line.loadIdentity();
      modelToWorld.translate(0, line_height, 0);
    }

    void pos_rotation(){
      modelToWorld.rotateZ(angle);
    }

    void neg_rotation(){
      modelToWorld.rotateZ(-angle);
    }

    void push(){
      modelToWorld_stack.push_back(modelToWorld);
    }

    void pop(){
      //draw a leaf when we're popping back
      branch l;
      l.leaf = modelToWorld;
      branches.push_back(l);
      l.leaf.loadIdentity();
      modelToWorld = modelToWorld_stack[modelToWorld_stack.size() - 1];
      modelToWorld_stack.pop_back();
    }

    void build_tree(std::string lsys_output){

      modelToWorld.loadIdentity();
      for (int i = 0; i < lsys_output.length(); ++i){
        if (lsys_output[i] == 'F'){
          forward();
        }
        if (lsys_output[i] == '+'){
          pos_rotation();
        }
        if (lsys_output[i] == '-'){
          neg_rotation();
        }
        if (lsys_output[i] == '['){
          push();
        }
        if (lsys_output[i] == ']'){
          pop();
        }
      }
    }

    /// this is called once OpenGL is initialized
    void app_init() {
      app_scene = new visual_scene();
      app_scene->create_default_camera_and_lights();
      app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(vec3(0.0f, 200.0f, 300.0f));

      line_shader.init();

      open_file("../../../assets/l-sys1.txt"); //load up our first text file
      generate_tree();
    }

    void open_file(std::string txtfile){

      std::ifstream file(txtfile);
      std::string lsystem_name;
      //if exists
      if (file.is_open()){

        printf("reading file...\n");
        std::getline(file, lsystem_name);

        std::string newline;
        while (std::getline(file, newline)){
          if (newline.compare("Iterations:") == 0){
            std::getline(file, newline);
            max_iterations = std::stoi(newline); //convert the string into int
            iterations = max_iterations;
          }

          if (newline.compare("Angle:") == 0){
            std::getline(file, newline);
            angle = std::stof(newline); //convert string to float
          }

          if (newline.compare("Axiom:") == 0){
            std::getline(file, newline);
            axiom = newline;
          }
          if (newline.compare("Constants:") == 0){
            std::getline(file, newline);
            for (int i = 0; i < newline.length(); ++i){
              rules.push_back(std::string(&newline[i], &newline[i] + 1)); //convert the strings into chars
            }
          }

          if (newline.compare("Leaves") == 0){
            generate_leaves = true;
          }

          if (newline.compare("Rule:") == 0){
            std::getline(file, newline);
            rules.push_back(newline);
          }
        }
        file.close(); //don't need the file anymore
        printf("%s file has been read\n", lsystem_name.c_str()); //print the name of the file
      }
      else{
        printf("file not found\n");
      }
    }


    ///Generate the l-system grammar based on the text files inputs, i.e. how many iterations and the given rules
    void generate_tree(){

      std::string append_iteration; //temp string to store our new iterations
      current_iteration = axiom; //we're starting with the axiom first

      for (int i = 0; i < iterations; ++i){ //maximum iterations for the l-system
        for (int j = 0; j < current_iteration.length(); ++j){ //read the string of the current iteration
          for (unsigned k = 0; k < rules.size(); ++k){ //for the amount of rules we have 
            if (current_iteration[j] == rules[k][0]){ //check the first character of the rule
              if (isalpha(rules[k][0])){ //check if its in the alphabet, the constants []+- will be omitted
                append_iteration.append(rules[k].substr(3)); //add the rule  
              }
              else{
                append_iteration.append(rules[k]); //if []+- has been found
              }
            }
          }
        }
        current_iteration = append_iteration; //store data from temp iteration into current_iteration array
        append_iteration.clear(); //clear the appended one for the next iteration
      }
      build_tree(current_iteration); //build the tree on the last iteration
    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {

      //taken from andy's triangle example
      glClearColor(0, 0, 0, 1); //black
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      hotkeys(); //hotkeys

      draw_tree(); //update tree
    }

    void hotkeys(){

      //1-8 number keys on the keyboard
      for (int i = 1; i <= 8; ++i){
        char c = i + '0'; //ascii value
        if (is_key_going_down(c)){
          clear_data();
          std::string txtfile = "../../../assets/l-sysn.txt";
          txtfile.replace(21, 1, std::to_string(i)); //just inject the number of the key into the string and read that text file
          open_file(txtfile); //load up text file
          generate_tree();
          adjust_camera(i);
        }
      }

      if (is_key_down('W')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0.0f, 10.0f, 0.0f);
      }
      if (is_key_down('S')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0.0f, -10.0f, 0.0f);
      }
      if (is_key_down('A')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(-10.0f, 0.0f, 0.0f);
      }
      if (is_key_down('D')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(10.0f, 0.0f, 0.0f);
      }
      if (is_key_down('Q')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0.0f, 0.0f, -10.0f);
      }
      if (is_key_down('E')){
        app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0.0f, 0.0f, 10.0f);
      }



      //step through the iterations 
      if (is_key_going_down(key_up)){
        if (iterations < max_iterations){
          branches.reset(); //reset our lines - gets ugly if we dont remove these!!
          current_iteration.clear();
          iterations++;
          generate_tree();
        }
      }
      if (is_key_going_down(key_down)){
        if (iterations > 0){
          branches.reset();//reset our lines - gets ugly if we dont remove these!!
          current_iteration.clear();
          iterations--;
          generate_tree();
        }
      }

      //toggle leaves
      if (is_key_going_down('L')){
        generate_leaves = !generate_leaves;
        branches.reset();
        build_tree(current_iteration);
      }

      //colour schemes

      if (is_key_going_down('H')){
        line_colour = vec4(0.2f, 1.0f, 0.0f, 1.0f);
        leaf_colour = vec4(0.3f, 0.0f, 1.0f, 1.0f);
        branches.reset();
        build_tree(current_iteration);
      }

      //brown and green
      if (is_key_going_down('J')){
        line_colour = vec4(0.2f, 0.098f, 0.0f, 1.0f);
        leaf_colour = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        branches.reset();
        build_tree(current_iteration);
      }

      if (is_key_going_down('K')){
        line_colour = vec4(0.2f, 0.0f, 1.0f, 1.0f);
        leaf_colour = vec4(0.8f, 0.4f, 0.0f, 1.0f);
        branches.reset();
        build_tree(current_iteration);
      }


      //change the width
      if (is_key_going_down('Z')){
        if (line_width < line_w_max){
          branches.reset();
          line_width += 1.0f;
          build_tree(current_iteration);
        }
      }
      if (is_key_going_down('X')){
        if (line_width > 0.0f){
          branches.reset();
          line_width -= 1.0f;
          build_tree(current_iteration);
        }
      }

      //change the angle
      if (is_key_down(key_left)){
        branches.reset();
        angle += 1.0f;
        build_tree(current_iteration);
      }
      if (is_key_down(key_right)){
        branches.reset();
        angle -= 1.0f;
        build_tree(current_iteration);
      }

      //close the application
      if (is_key_down(key_esc)){
        exit(0);
      }
    }

    //adjust the camera to see the different l-systems (they have different sizes so lets accomodate for that)
    void adjust_camera(int i){
      mat4t &camera = app_scene->get_camera_instance(0)->get_node()->access_nodeToParent();
      camera.loadIdentity();
      if (i == 1){
        camera.translate(vec3(0.0f, 200.0f, 300.0f));
      }
      if (i == 2 || i == 3){
        camera.translate(vec3(0.0f, 50.0f, 80.0f));
      }
      if (i == 4 || i == 5){
        camera.translate(vec3(0.0f, 250.0f, 300.0f));
      }
      if (i == 6){
        camera.translate(vec3(0.0f, 70.0f, 100.0f));
      }
      if (i == 7){
        camera.translate(vec3(20.0f, -40.0f, 100.0f));
      }
      if (i == 8){
        camera.translate(vec3(-70.0f, 60.0f, 160.0f));
      }
    }

    //clear the data for the next l-system to load in without any problems
    void clear_data(){
      modelToWorld.loadIdentity();
      max_iterations = 0;
      angle = 0.0f;
      rules.reset();
      branches.reset();
      axiom.clear();
      current_iteration.clear();
      generate_leaves = false;
    }

  };
}
