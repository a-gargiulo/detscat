import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


def draw_filled_circle(ax, center, normal, radius, color='skyblue', alpha=0.4, n_points=60):
    center = np.asarray(center)
    normal = np.asarray(normal) / np.linalg.norm(normal)

    # Find orthogonal basis vectors
    if np.allclose(normal, [0,0,1]):
        not_parallel = np.array([1,0,0])
    else:
        not_parallel = np.array([0,0,1])
    u = np.cross(normal, not_parallel); u /= np.linalg.norm(u)
    v = np.cross(normal, u)

    theta = np.linspace(0, 2*np.pi, n_points)
    circle = center + radius * (np.outer(np.cos(theta), u) + np.outer(np.sin(theta), v))

    # Add filled surface
    ax.add_collection3d(Poly3DCollection(
        [circle], color=color, alpha=alpha, edgecolor='k'
    ))



def plot_circle_3d(ax, center, normal, radius=1.0, color='C0', n_points=200):
    center = np.asarray(center)
    normal = np.asarray(normal)
    normal = normal / np.linalg.norm(normal)

    # Pick an arbitrary vector not parallel to normal
    if np.allclose(normal, [0,0,1]):
        not_parallel = np.array([1,0,0])
    else:
        not_parallel = np.array([0,0,1])

    # Build two orthogonal vectors in the plane
    u = np.cross(normal, not_parallel)
    u /= np.linalg.norm(u)
    v = np.cross(normal, u)

    # Parametric circle
    theta = np.linspace(0, 2*np.pi, n_points)
    circle_points = center[:,None] + radius*(np.outer(u, np.cos(theta)) + np.outer(v, np.sin(theta)))

    ax.plot(circle_points[0], circle_points[1], circle_points[2], color=color, linewidth=2)







data = np.loadtxt("basis.txt")
pos = np.loadtxt("positions.txt")

# TUNNEL system
tBt = data[0:3, :]  # Tunnel basisi in tunnel coordinates

# WORLD system
tBw = data[3:6, :]  # World basis in tunnel coordinates
Rwt = data[6:9, :]  # Rotation Tunnel -> World
wTwt = data[9, :]   # From centroid to tunnel origin in world coordinates
tTwt = -Rwt.T @ wTwt    # From tunnel to centroid in tunnel coordinates 

# CAMERA system
Rcw = data[10:13, :]
cTcw = data[13, :]  # from camera center to world origin in camera coordinates
wTcw = -Rcw.T @ cTcw
tTcw = Rwt.T @ wTcw
tTct = tTwt + tTcw

tBc = (Rcw @ Rwt).T @ tBt


theta = 1.496345
phi = -1.508862
cPa = np.array([-0.018750, -0.018750, 0.000000])
wPa = Rcw.T @ (cPa - cTcw)
tPa = Rwt.T @ (wPa - wTwt)

wNs = np.array([np.cos(theta), np.sin(theta) * np.cos(phi), np.sin(theta) * np.sin(phi)])
wNperp = np.array([0.0, -np.sin(phi), np.cos(phi)])
wNpar= np.array([-np.sin(theta), np.cos(theta) * np.cos(phi), np.cos(theta) * np.sin(phi)])

Rcs = np.array(
    [
        [-np.sin(theta), 0, np.cos(theta)],
        [np.cos(theta) * np.cos(phi), -np.sin(phi), np.sin(theta) * np.cos(phi)],
        [np.cos(theta) * np.sin(phi), np.cos(phi), np.sin(theta) * np.sin(phi)]
    ])
wa1 = Rcs.T @ wNpar
wa2 = Rcs.T @ wNperp
wa3 = Rcs.T @ wNs

wb1 = Rcw.T @  wa1
wb2 = Rcw.T @  wa2
wb3 = Rcw.T @  wa3


fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

ax.scatter(tPa[0], tPa[1], tPa[2], c="r", s=25)
labels = ['par', 'perp', 'r']
for v, l in zip([Rwt.T @ wNpar, Rwt.T @ wNperp, Rwt.T @ wNs], labels):
    ax.quiver(tPa[0], tPa[1], tPa[2], v[0], v[1], v[2], color='m', linewidth=2,
              length=0.1, arrow_length_ratio=0.1)
    tip = tPa + v / np.linalg.norm(v) * 0.12
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='m', ha='center', va='center')

labels = ['x', 'y', 'z']
for v, l in zip([Rwt.T @ wa1, Rwt.T @ wa2, Rwt.T @ wa3], labels):
    ax.quiver(tPa[0], tPa[1], tPa[2], v[0], v[1], v[2], color='k', linewidth=2,
              length=0.1, arrow_length_ratio=0.1)
    tip = tPa + v / np.linalg.norm(v) * 0.12
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='k', ha='center', va='center')

labels = ['x', 'y', 'z']
for v, l in zip([Rwt.T @ wb1, Rwt.T @ wb2, Rwt.T @ wb3], labels):
    ax.quiver(tPa[0], tPa[1], tPa[2], v[0], v[1], v[2], color='cyan', linewidth=2,
              length=0.1, arrow_length_ratio=0.1)
    tip = tPa + v / np.linalg.norm(v) * 0.12
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='cyan', ha='center', va='center')

ax.plot(
    [tTwt[0], tPa[0]],
    [tTwt[1], tPa[1]],
    [tTwt[2], tPa[2]],
    color='blue', linewidth=2
)


Dcircle = 0.13335



# Define box corners (example: 0..1 cube)
x = [-2.286, 1.3716]
y = [-0.2032 / 2.0, 0.2032 / 2.0]
z = [-0.01905, 0.01905]

# Vertices of the box
vertices = np.array([[x[0], y[0], z[0]],
                     [x[1], y[0], z[0]],
                     [x[1], y[1], z[0]],
                     [x[0], y[1], z[0]],
                     [x[0], y[0], z[1]],
                     [x[1], y[0], z[1]],
                     [x[1], y[1], z[1]],
                     [x[0], y[1], z[1]]])

# Define faces by vertex indices
faces = [[vertices[j] for j in [0,1,2,3]],  # bottom
         [vertices[j] for j in [4,5,6,7]],  # top
         [vertices[j] for j in [0,1,5,4]],  # front
         [vertices[j] for j in [2,3,7,6]],  # back
         [vertices[j] for j in [1,2,6,5]],  # right
         [vertices[j] for j in [0,3,7,4]]]  # left

# Add as filled surfaces
ax.add_collection3d(Poly3DCollection(
    faces, facecolors='gray', linewidths=1, edgecolors='k', alpha=0.2
))

# draw_filled_circle(ax, center=[0.0, 0.0, z[0]], normal=[0, 0, 1], radius=Dcircle/2.0, color='skyblue', alpha=0.2, n_points=60)
# draw_filled_circle(ax, center=[0.0, 0.0, z[1]], normal=[0, 0, 1], radius=Dcircle/2.0, color='skyblue', alpha=0.5, n_points=60)




plot_circle_3d(ax, center=[0.0, 0.0, z[0]], normal=[0, 0, 1], radius=Dcircle/2.0, color='k', n_points=200)
plot_circle_3d(ax, center=[0.0, 0.0, z[1]], normal=[0, 0, 1], radius=Dcircle/2.0, color='k', n_points=200)




labels = ['x', 'y', 'z']
for v, l in zip(tBt, labels):
    ax.quiver(0, 0, 0, v[0], v[1], v[2], color='b', linewidth=2,
              length=0.01, arrow_length_ratio=0.1)
    tip = np.array([0, 0, 0]) + v / np.linalg.norm(v) * 0.012
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='b', ha='center', va='center')

for v, l in zip(tBw, labels):
    ax.quiver(tTwt[0], tTwt[1], tTwt[2], v[0], v[1], v[2], color='r', linewidth=2,
              length=0.01, arrow_length_ratio=0.1)
    tip = tTwt + v / np.linalg.norm(v) * 0.012
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='r', ha='center', va='center')

for v, l in zip(tBc, labels):
    ax.quiver(tTct[0], tTct[1], tTct[2], v[0], v[1], v[2], color='g', linewidth=2,
              length=0.1, arrow_length_ratio=0.1)
    tip = tTct + v / np.linalg.norm(v) * 0.12
    ax.text(tip[0], tip[1], tip[2], l, fontsize=12, color='g', ha='center', va='center')

plot_circle_3d(ax, center=[tTct[0], tTct[1], tTct[2]], normal=tBc[2], radius=(0.3/8)/2.0, color='k', n_points=200)

for p in range(len(pos) - 1):
    ax.scatter(pos[p, 0], pos[p, 1], pos[p, 2], c="black")
ax.scatter(pos[-1, 0], pos[-1, 1], pos[-1, 2], c="orange", s=100)

ax.set_xlim([-0.8, 0.8])
ax.set_ylim([-0.8, 0.8])
ax.set_zlim([-0.8, 0.8])

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')

ax.set_box_aspect([1, 1, 1])
ax.view_init(elev=30, azim=160, roll=-100)

plt.show()
