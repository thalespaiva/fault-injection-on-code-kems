import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from matplotlib import rc
from matplotlib import rcParams

from ast import literal_eval

TCHES_FIG_WIDTH = 5.39
TCHES_SMALL_FIG_HEIGHT = 2.2

TCHES_HALF_PAGE_FIG = 2.2
TCHES_HALF_PAGE_FIG_BIG = 2.4
TCHES_HALF_PAGE_FIG_SMALL = 2

def latexify_tches(fig_width=TCHES_FIG_WIDTH, fig_height=TCHES_SMALL_FIG_HEIGHT, small=False, fs=8):

    sns.reset_orig()

    rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})
    rc('text', usetex=True)
    rcParams['font.size'] = fs
    from math import sqrt

    if fig_width is None:
        fig_width = 4.8  # approx 12.2 cm

    if fig_height is None:
        golden_mean = (sqrt(5) - 1.0) / 2.0  # Aesthetic ratio
        fig_height = fig_width * golden_mean  # height in inches

    MAX_HEIGHT_INCHES = 8.0
    if fig_height > MAX_HEIGHT_INCHES:
        print("WARNING: fig_height too large:" + fig_height +
              "so will reduce to" + MAX_HEIGHT_INCHES + "inches.")
        fig_height = MAX_HEIGHT_INCHES

    # ticksize = fs
    ticksize = 7
    if small:
        ticksize = 7

    params = {
        'backend': 'pdf',
        'axes.labelsize': fs,  # fontsize for x and y labels (was 10)
        'axes.titlesize': fs,
        'font.size': fs,  # was 10
        'legend.fontsize': fs,  # was 10
        'font.family': 'serif',
        'xtick.labelsize': ticksize,
        'ytick.labelsize': ticksize,
        'ytick.major.width': 0.3,
        'ytick.minor.width': 0.3,
        'xtick.major.width': 0.3,
        'xtick.minor.width': 0.3,
        'text.usetex': True,
        'legend.edgecolor': 'none',
        'legend.frameon': True,
        'legend.framealpha': 0.5,
        'legend.fancybox': False,
        'axes.linewidth': 0.3,
        'axes.linewidth': 0.3,
        'figure.figsize': [fig_width, fig_height],
    }

    rcParams.update(params)


def plot_data_bike_fia_effect_on_threshold(datafile='data/data_bike_fia_effect_on_threshold.csv'):

    df = pd.read_csv(datafile)
    fig, axs = plt.subplots(1, 2, figsize=(TCHES_FIG_WIDTH, TCHES_SMALL_FIG_HEIGHT))

    df1 = df[(df.level == 1) & (df.bgf_faulted_iteration == 2)]

    palette = ['0.6', '0.0']

    sns.histplot(data=df1, x='global_synd_weight', hue='failed', element='step', palette=palette, ax=axs[0])
    sns.despine(ax=axs[0])
    df5 = df[(df.level == 5) & (df.bgf_faulted_iteration == 1)]
    sns.histplot(data=df5, x='global_synd_weight', hue='failed', element='step', palette=palette, ax=axs[1])
    sns.despine(ax=axs[1])

    axs[0].set_title('BIKE Level 1')
    axs[1].set_title('BIKE Level 5')

    axs[1].set_xlim(16500, 17400)


    for ax in axs:
        ax.set_xlabel('Weight of secret syndrome')
        leg = ax.get_legend()
        leg.set_title("Decaps failed?")

        # for text, new_label in zip(leg.texts, ["False", "True"]):
        #     text.set_text(new_label)

        leg.set_loc('upper right')

    plt.tight_layout()
    plt.savefig('graphs/figs/bike_fia_effect_on_threshold.pdf')



def parse_fia_simulation_csv(datafile):
    list_converter = lambda x: map(int, )

    df_xy = pd.read_csv(datafile, converters={
        "x": literal_eval,
        "y": literal_eval,
    }, nrows=1)

    x = df_xy.x[0]
    y = df_xy.y[0]

    df = pd.read_csv(datafile, skiprows=2)

    return (x, y), df

def plot_key_relation_for_fia_schoolbook_mul(datafile='../results/hqc/1fault_schoolbook_mul/hqc1_seed40.csv'):
    (x, y), df = parse_fia_simulation_csv(datafile)

    fig, ax = plt.subplots(1, 1, figsize=(TCHES_FIG_WIDTH, 1.3))
    # fig, axs = plt.subplots(6, 1)

    sns.lineplot(data=df, x='position', y='failure_rate', lw=0.3, ax=ax, c='0')
    for i, v in enumerate(y):
        ax.axvline(v, c='0.4', lw=0.3, ls='--')
        plt.plot(v, 1.2, marker='x', markersize=3, c='0', ls='None',
            label=r'Support of $\textbf{y}$' if i == 0 else None
        )
    # ax.set_xlim(7350, 7600)
    plt.xlim(4245, 4512)
    ax.set_ylim(-0.5, 1.5)
    ax.set_yticks([0, 1])
    ax.set_ylabel('Decaps failed?')
    ax.set_xlabel(r'Fault index')
    sns.despine()
    leg = plt.legend(handletextpad=0.2, frameon=True, edgecolor='black', handlelength=0.6, loc='center right')
    leg.get_frame().set_linewidth(0.3)
    plt.tight_layout()




def plot_spectrum_bike(datafile='results/bike/bike1_seed1_15M.csv.xz'):
    df = pd.read_csv(datafile)

    fig, ax = plt.subplots(1, 1, figsize=(TCHES_FIG_WIDTH, TCHES_SMALL_FIG_HEIGHT))

    palette = ['1', '0.8', '0.5', '0.2', '0']
    sns.histplot(data=df[df.n_challenges == 3000000], x='spec_e0_estimator', multiple="stack", hue='spec_h0', binwidth=0.0001,
                 palette=palette)
    plt.xlabel('Decryption failure rate for cyclic distance')
    leg = plt.gca().get_legend()
    leg.set_title('Multiplicity in\nthe secret spectrum')
    leg.get_title().set_multialignment('center')
    sns.despine()
    plt.tight_layout()

    # plt.savefig('paper/figs/tp_figs/spectrum_bike_3M.pdf')



def plot_spectrum_hqc(datafile='results/hqc/1fault_vect_add/hqc1_seed1_50M_vect_add_1fault.csv.xz'):
    (x, y), df = parse_fia_simulation_csv(datafile)
    df = df[df.n_challenges == 47000000]

    fig, ax = plt.subplots(1, 1, figsize=(TCHES_FIG_WIDTH, TCHES_SMALL_FIG_HEIGHT))

    palette = ['1', '0.8', '0.5', '0.2', '0']

    u = np.sort(np.unique(df.spec_r1_estimator))
    mid = (u[:-1] + u[1:]) / 2
    edges = np.r_[u[0] - (mid[0] - u[0]), mid, u[-1] + (u[-1] - mid[-1])]

    sns.histplot(data=df, x='spec_r1_estimator', multiple="stack", hue='spec_y', bins=edges,
                 palette=palette, binwidth=0.00002)
    plt.xlabel('Decryption failure rate for cyclic distance')
    leg = plt.gca().get_legend()
    leg.set_title('Multiplicity in\nthe secret spectrum')
    leg.get_title().set_multialignment('center')
    sns.despine()
    plt.tight_layout()

    # plt.savefig('paper/figs/tp_figs/spectrum_hqc_50M.pdf')
    plt.savefig('hqc_spectrum_hqc_50M2.pdf')


def plot_spectrum_hqc_half(datafile='results/hqc/1fault_vect_add/hqc1_seed1_50M_vect_add_1fault.csv.xz'):
    (x, y), df = parse_fia_simulation_csv(datafile)
    df = df[df.n_challenges == 47000000]

    fig, ax = plt.subplots(1, 1, figsize=(TCHES_FIG_WIDTH/2, TCHES_SMALL_FIG_HEIGHT/1.5))

    palette = ['1', '0.8', '0.5', '0.2', '0']

    u = np.sort(np.unique(df.spec_r1_estimator))
    mid = (u[:-1] + u[1:]) / 2
    edges = np.r_[u[0] - (mid[0] - u[0]), mid, u[-1] + (u[-1] - mid[-1])]

    sns.histplot(data=df, x='spec_r1_estimator', multiple="stack", hue='spec_y', bins=edges,
                 palette=palette, binwidth=0.00003)
    # plt.xlim(0.3557, 0.35723)
    # plt.xlim(0.3556, 0.35723)
    plt.xlim(0.3555, 0.3573)
    # plt.xticks([0.3556, 0.35723])
    plt.xlabel('Decryption failure rate for cyclic distance')
    leg = plt.gca().get_legend()
    leg.set_title('Multiplicity in\nsecret spectrum')
    leg.get_title().set_multialignment('center')
    sns.despine()
    plt.tight_layout()

    fig.subplots_adjust(
        left=0.17,
        right=0.97,
        bottom=0.25,
        top=0.98
    )



    # plt.savefig('paper/figs/tp_figs/spectrum_hqc_50M.pdf')
    plt.savefig('paper/figs/tp_figs/hqc_spectrum_hqc_50M_half.pdf')


def plot_spectrum_bike_half(datafile='results/bike/bike-results-original/bike1_seed10_FIAerror0.004_5M.csv.xz'):
    df = pd.read_csv(datafile)

    fig, ax = plt.subplots(1, 1, figsize=(TCHES_FIG_WIDTH/2, TCHES_SMALL_FIG_HEIGHT/1.5))

    palette = ['1', '0.8', '0.5', '0.2', '0']
    sns.histplot(data=df[df.n_challenges == 3000000], x='spec_e0_estimator', multiple="stack", hue='spec_h0', binwidth=0.0002,
                 palette=palette)
    plt.xlabel('Decryption failure rate for cyclic distance')
    leg = plt.gca().get_legend()
    leg.set_title('Multiplicity in\nsecret spectrum')
    leg.get_title().set_multialignment('center')
    sns.despine()
    plt.tight_layout()

    fig.subplots_adjust(
        left=0.17,
        right=0.97,
        bottom=0.25,
        top=0.98
    )


    plt.savefig('paper/figs/tp_figs/spectrum_bike_3M_half.pdf')
