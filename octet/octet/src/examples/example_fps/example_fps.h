////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
namespace octet {
  /// Scene containing a box with octet.
  class example_fps : public app {
    // scene for drawing box
    ref<visual_scene> app_scene;

    mouse_look mouse_look_helper;

    ref<scene_node> player_node;

    ref<camera_instance> the_camera;

    // a class derived from mesh box that scales its vertices.
    class mesh_heightfield : public mesh {
      ivec3 dimensions;
    public:
      // construct using an empty box and then set mesh.
      mesh_heightfield(vec3_in size, ivec3_in dimensions) : mesh(), dimensions(dimensions) {
        set_default_attributes();
        set_aabb(aabb(vec3(0, 0, 0), size));
        update();
      }

      // override the update function to draw different geometry.
      void update() {
        dynarray<mesh::vertex> vertices;
        dynarray<uint32_t> indices;

        vertices.reserve((dimensions.x()+1) * (dimensions.z()+1));

        vec3 dimf = (vec3)(dimensions);
        aabb bb = get_aabb();
        vec3 bb_min = bb.get_min();
        vec3 bb_delta = bb.get_half_extent() / dimf * 2.0f;
        vec3 uv_min = vec3(0);
        vec3 uv_delta = vec3(30.0f/dimf.x(), 30.0f/dimf.z(), 0);
        vec3 bumps[] = { vec3(100, 0, 100) };
        for (int x = 0; x <= dimensions.x(); ++x) {
          for (int z = 0; z <= dimensions.z(); ++z) {
            vec3 xz = vec3((float)x, 0, (float)z) * bb_delta;
            float y = expf((xz - bumps[0]).squared() / (-1000.0f)) * 10.0f - 10.0f;
            //float y = sinf(xz.x() * 0.01f) * 1.0f + sinf(xz.z() * 0.03f) * 0.5f;
            float dy_dx = cosf(xz.x() * 0.01f);
            float dy_dz = cosf(xz.z() * 0.03f);
            vec3 pos = bb_min + xz + vec3(0, y, 0);
            vec3 normal = normalize(vec3(dy_dx, 1, dy_dz));
            vec3 uv = uv_min + vec3((float)x, (float)z, 0) * uv_delta;
            vertices.push_back(mesh::vertex(pos, normal, uv));
          }
        }

        indices.reserve(dimensions.x() * dimensions.z() * 6);

        int stride = dimensions.x() + 1;
        for (int x = 0; x < dimensions.x(); ++x) {
          for (int z = 0; z < dimensions.z(); ++z) {
            // 01 11
            // 00 10
            indices.push_back((x+0) + (z+0)*stride);
            indices.push_back((x+0) + (z+1)*stride);
            indices.push_back((x+1) + (z+0)*stride);
            indices.push_back((x+1) + (z+0)*stride);
            indices.push_back((x+0) + (z+1)*stride);
            indices.push_back((x+1) + (z+1)*stride);
          }
        }

        set_vertices(vertices);
        set_indices(indices);
      }
    };

    void update_keys(scene_node *player_node, scene_node *camera_node) {
      float friction = 0.0f;
      if (is_key_down('A')) {
        player_node->apply_central_force(camera_node->get_x() * (-1000.0f));
      } else if (is_key_down('D')) {
        player_node->apply_central_force(camera_node->get_x() * (+1000.0f));
      } else if (is_key_down('W')) {
        player_node->apply_central_force(camera_node->get_z() * (-1000.0f));
      } else if (is_key_down('S')) {
        player_node->apply_central_force(camera_node->get_z() * (+1000.0f));
      } else {
        friction = 100.0f;
      }
      if (is_key_going_down(' ')) {
        player_node->apply_central_force(camera_node->get_y() * (+10000.0f));
      }
      player_node->set_friction(friction);
    }
  public:
    /// this is called when we construct the class before everything is initialised.
    example_fps(int argc, char **argv) : app(argc, argv) {
    }

    /// this is called once OpenGL is initialized
    void app_init() {
      mouse_look_helper.init(this, 200.0f / 360.0f, false);
      app_scene =  new visual_scene();
      app_scene->create_default_camera_and_lights();
      the_camera = app_scene->get_camera_instance(0);
      the_camera->get_node()->translate(vec3(0, 4, 0));
      the_camera->set_far_plane(10000);

      mat4t mat;

      mat.loadIdentity();
      mat.translate(0, -0.5f, 0);

      app_scene->add_shape(
        mat,
        new mesh_heightfield(vec3(100.0f, 0.5f, 100.0f), ivec3(100, 1, 100)),
        new material(new image("assets/grass.jpg")),
        false, 0
      );

      float player_height = 1.83f;
      float player_radius = 0.25f;
      float player_mass = 90.0f;

      mat.loadIdentity();
      mat.translate(0, player_height*0.5f, 0);

      mesh_instance *mi = app_scene->add_shape(
        mat,
        new mesh_sphere(vec3(0), player_radius),
        new material(vec4(0, 0, 1, 1)),
        true, player_mass//,
        //new btCapsuleShape(0.25f, player_height)
      );
      player_node = mi->get_node();

      player_node->set_friction(0);
      player_node->set_damping(0, 100);

      //app_scene->set_render_aabbs(true);
      //app_scene->add_debug_line(vec3(-100, 0, 0), vec3(100, 0, 0));
      //app_scene->set_render_debug_lines(true);
    }

    /// this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
      int vx = 0, vy = 0;
      get_viewport_size(vx, vy);
      app_scene->begin_render(vx, vy);

      scene_node *camera_node = the_camera->get_node();
      mat4t &camera_to_world = camera_node->access_nodeToParent();
      mouse_look_helper.update(camera_to_world);

      update_keys(player_node, camera_node);

      //player_node->apply_central_force(vec3(-100, 0, 0));

      //char tmp[256];  
      //printf("player_node->get_position()=%s\n", player_node->get_position().toString(tmp, 256));
      camera_to_world.w() = (player_node->get_position() + vec3(0, 1.25f , 0) ).xyz1();

      // update matrices. assume 30 fps.
      app_scene->update(1.0f/30);

      player_node->set_angular_velocity(vec3(0, 0, 0));
      player_node->set_rotation(mat4t());

      // draw the scene
      app_scene->render((float)vx / vy);
    }
  };
}
