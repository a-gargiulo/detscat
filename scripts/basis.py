import matplotlib.pyplot as plt
import numpy as np
import numpy.typing as npt

from mpl_toolkits.mplot3d.art3d import Poly3DCollection  # type: ignore
from mpl_toolkits.mplot3d.axes3d import Axes3D  # type: ignore


def plot_filled_circle_3d(
    ax: Axes3D,
    center: npt.NDArray[np.float64],
    normal: npt.NDArray[np.float64],
    radius: float,
    color: str = "red",
    alpha: float = 0.5,
    n_points: int = 200,
):
    center = np.asarray(center)
    normal = np.asarray(normal) / np.linalg.norm(normal)

    if np.allclose(normal, [0, 0, 1]):
        not_parallel = np.array([1, 0, 0])
    else:
        not_parallel = np.array([0, 0, 1])
    u = np.cross(normal, not_parallel)
    u /= np.linalg.norm(u)
    v = np.cross(normal, u)

    theta = np.linspace(0, 2 * np.pi, n_points)
    circle = center + radius * (
        np.outer(np.cos(theta), u) + np.outer(np.sin(theta), v)
    )

    ax.add_collection3d(
        Poly3DCollection([circle], color=color, alpha=alpha, edgecolor="k")
    )


def plot_circle_3d(
    ax: Axes3D,
    center: npt.NDArray[np.float64],
    normal: npt.NDArray[np.float64],
    radius: float = 1.0,
    color: str = "black",
    n_points: int = 200,
):
    center = np.asarray(center)
    normal = np.asarray(normal)
    normal = normal / np.linalg.norm(normal)

    if np.allclose(normal, [0, 0, 1]):
        not_parallel = np.array([1, 0, 0])
    else:
        not_parallel = np.array([0, 0, 1])

    u = np.cross(normal, not_parallel)
    u /= np.linalg.norm(u)
    v = np.cross(normal, u)

    theta = np.linspace(0, 2 * np.pi, n_points)
    circle_points = center[:, None] + radius * (
        np.outer(u, np.cos(theta)) + np.outer(v, np.sin(theta))
    )

    ax.plot(
        circle_points[0],
        circle_points[1],
        circle_points[2],
        color=color,
        linewidth=2,
    )


def plot_detonation_tunnel(ax: Axes3D):
    x = [-2.286, 1.3716]
    y = [-0.2032 / 2.0, 0.2032 / 2.0]
    z = [-0.01905, 0.01905]

    D_window = 0.13335

    vertices = np.array(
        [
            [x[0], y[0], z[0]],
            [x[1], y[0], z[0]],
            [x[1], y[1], z[0]],
            [x[0], y[1], z[0]],
            [x[0], y[0], z[1]],
            [x[1], y[0], z[1]],
            [x[1], y[1], z[1]],
            [x[0], y[1], z[1]],
        ]
    )

    faces = [
        [vertices[j] for j in [0, 1, 2, 3]],
        [vertices[j] for j in [4, 5, 6, 7]],
        [vertices[j] for j in [0, 1, 5, 4]],
        [vertices[j] for j in [2, 3, 7, 6]],
        [vertices[j] for j in [1, 2, 6, 5]],
        [vertices[j] for j in [0, 3, 7, 4]],
    ]

    ax.add_collection3d(
        Poly3DCollection(
            faces, facecolors="gray", linewidths=1, edgecolors="k", alpha=0.2
        )
    )

    plot_circle_3d(
        ax,
        center=np.array([0.0, 0.0, z[0]]),
        normal=np.array([0, 0, 1]),
        radius=D_window / 2.0,
        color="k",
        n_points=200,
    )
    plot_circle_3d(
        ax,
        center=np.array([0.0, 0.0, z[1]]),
        normal=np.array([0, 0, 1]),
        radius=D_window / 2.0,
        color="k",
        n_points=200,
    )


def plot_basis(
    ax: Axes3D,
    basis: npt.NDArray[np.float64],
    origin: npt.NDArray[np.float64],
    labels: list[str],
    color: str = "blue",
    length: float = 0.1,
    arrow_length_ratio: float = 0.1,
):
    for v, l in zip(basis, labels):
        ax.quiver(
            origin[0],
            origin[1],
            origin[2],
            v[0],
            v[1],
            v[2],
            color=color,
            linewidth=2,
            length=length,
            arrow_length_ratio=arrow_length_ratio,
        )
        tip = origin + v / np.linalg.norm(v) * length * 1.2
        ax.text(
            tip[0],
            tip[1],
            tip[2],
            l,
            fontsize=12,
            color=color,
            ha="center",
            va="center",
        )



def print_basis(basis: npt.NDArray[np.float64], labels: list[str]):
    for v, l in zip(basis, labels):
        print(f"\t{l}: {v[0]:10.5f}, {v[1]:10.5f}, {v[2]:10.5f}")
    print("")


if __name__ == "__main__":
    data = np.loadtxt("basis.txt")
    prt = np.loadtxt("positions.txt")

    print("TUNNEL BASIS (in tunnel coordinates):\n")
    tBt = data[0:3, :]
    print_basis(tBt, ["x", "y", "z"])

    print("WORLD BASIS (in tunnel coordinates):\n")
    tBw = data[3:6, :]
    print_basis(tBw, ["x", "y", "z"])
    Rwt = data[6:9, :]
    wTwt = data[9, :]  # From centroid to tunnel origin in world coordinates
    tTwt = -Rwt.T @ wTwt  # Centroid location in tunnel coordinates

    OW = tTwt  # WORLD SYSTEM ORIGIN
    BW = tBw   # WORLD SYSTEM AXES

    Rcw = data[10:13, :]
    cTcw = data[13, :]   # From camera center to world origin in camera coordinates

    wTcw = -Rcw.T @ cTcw  # PASSIVE transform to world and flip (so that world origin to camera center)
    tTcw = Rwt.T @ wTcw   # PASSIVE transform from world to tunnel
    tTct = tTwt + tTcw    # Total translation from tunnel origin to camera origin in tunnel coordinates

    tBc = np.zeros((3, 3))
    Rct = Rcw @ Rwt
    for i, v in enumerate(tBt):
        tBc[i, :] = Rct.T @ v  # ACTIVE transformation from tunnel to camera

    OC = tTct  # CAMERA SYSTEM ORIGIN
    BC = tBc   # CAMERA SYSTEM AXES

    # Sample point on aperture
    cPa = np.array([-0.018750, -0.018750, 0.000000])
    # Corresponding scattering direction from WORLD coordinate system
    theta = 1.496345
    phi = -1.508862

    wPa = Rcw.T @ (cPa - cTcw)  # PASSIVE transformation from camera to world
    tPa = Rwt.T @ (wPa - wTwt)  # PASSIVE transformation from world to tunnel

    wNs = np.array([
        np.cos(theta),
        np.sin(theta) * np.cos(phi),
        np.sin(theta) * np.sin(phi)
    ])
    wNperp = np.array([0.0, -np.sin(phi), np.cos(phi)])
    wNpar = np.array([
        -np.sin(theta),
        np.cos(theta) * np.cos(phi),
        np.cos(theta) * np.sin(phi)
    ])

    # x->par, y->perp, z->r
    Rws = np.array(
        [
            [-np.sin(theta), 0, np.cos(theta)],
            [
                np.cos(theta) * np.cos(phi),
                -np.sin(phi),
                np.sin(theta) * np.cos(phi),
            ],
            [
                np.cos(theta) * np.sin(phi),
                np.cos(phi),
                np.sin(theta) * np.sin(phi),
            ],
        ]
    )

    tBs1 = Rwt.T @ wNpar  # PASSIVE rotation from world to tunnel
    tBs2 = Rwt.T @ wNperp  # PASSIVE rotation from world to tunnel
    tBs3 = Rwt.T @ wNs  # PASSIVE rotation from world to tunnel
    tBs = np.array([tBs1, tBs2, tBs3])

    OS = tPa  # SCATTERING SYSTEM ORIGIN
    BS = tBs  # SCATTERING SYSTEM AXES

    tBcLocal = np.zeros((3, 3))
    Rts = Rwt.T @ Rws
    for i, v in enumerate(tBs):
        v_tmp = Rts.T @ v  # ACTIVE transformation from scattering to tunnel
        tBcLocal[i, :] = Rct.T @ v_tmp  # ACTIVE transformation from tunnel to camera


    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")
    plot_detonation_tunnel(ax)

    # Aperture
    plot_circle_3d(
        ax,
        center=np.array([tTct[0], tTct[1], tTct[2]]),
        normal=tBc[2],
        radius=(0.3 / 8) / 2.0,
        color="black",
        n_points=200,
    )

    for p in range(len(prt) - 1):
        ax.scatter(prt[p, 0], prt[p, 1], prt[p, 2], c="black")
    ax.scatter(prt[-1, 0], prt[-1, 1], prt[-1, 2], c="orange", s=100)

    plot_basis(
        ax,
        tBt,
        np.array([0, 0, 0]),
        ["x", "y", "z"],
        color="red",
        length=0.01,
        arrow_length_ratio=0.1
    )

    plot_basis(
        ax,
        BW,
        OW,
        ["x", "y", "z"],
        color="blue",
        length=0.01,
        arrow_length_ratio=0.1
    )

    plot_basis(
        ax,
        BC,
        OC,
        ["x", "y", "z"],
        color="green",
        length=0.1,
        arrow_length_ratio=0.1
    )

    ax.scatter(tPa[0], tPa[1], tPa[2], c="magenta", s=25)

    plot_basis(
        ax,
        BS,
        OS,
        ["par", "perp", "r"],
        color="gray",
        length=0.1,
        arrow_length_ratio=0.1
    )

    ax.plot(
        [tTwt[0], tPa[0]],
        [tTwt[1], tPa[1]],
        [tTwt[2], tPa[2]],
        color="gray",
        linestyle=":",
        linewidth=2,
    )

    plot_basis(
        ax,
        tBcLocal,
        OS,
        ["x", "y", "z"],
        color="cyan",
        length=0.1,
        arrow_length_ratio=0.1
    )

    ax.set_xlim([-0.8, 0.8])
    ax.set_ylim([-0.8, 0.8])
    ax.set_zlim([-0.8, 0.8])

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")

    ax.set_box_aspect([1, 1, 1])
    ax.view_init(elev=30, azim=160, roll=-100)

    plt.show()
