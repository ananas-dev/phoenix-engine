\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{report}[2010/09/14 v0.2 ingi2261 assignment class]

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Options

\gdef\@docname{Report}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Class loading
\LoadClass[11pt,oneside,a4paper]{article}
% \RequirePackage[a4paper, total={6in, 8in}]{geometry}
\RequirePackage[utf8]{inputenc}
\RequirePackage[T1]{fontenc}
\RequirePackage[math]{iwona}
\RequirePackage[scaled=0.8]{beramono}
\RequirePackage{amsmath,amsfonts,amssymb,braket}
\RequirePackage{array}
\RequirePackage[hmargin=2cm,vmargin=2cm,includehead,includefoot]{geometry}
\RequirePackage{graphicx}
\RequirePackage{float,caption}
\RequirePackage{multicol,multirow}
\RequirePackage{chngpage}
\RequirePackage{listings}
\RequirePackage{enumitem}
\RequirePackage{subcaption}
\RequirePackage{caption}
\RequirePackage[pdftex,colorlinks=false,pdfborder={0 0 0}]{hyperref}
 \usepackage{environ}

\RequirePackage{tikz}
\let\svtikzpicture\tikzpicture
\def\tikzpicture{\noindent\svtikzpicture}
\RequirePackage{enumitem}
\setenumerate[2]{labelindent=0pt,itemindent=0pt}

\newcommand*\students[2]{
\newcommand{\studenta}{#1}
\newcommand{\studentb}{#2}
% \gdef\@studenta{#1} \gdef\@studentb{#2}
}
\newcommand*\group[1]{\newcommand{\groupnumber}{#1 (Moodle and Inginious)}}

\newcommand*\assignment[1]{\gdef\@assignment{#1}}
\gdef\@assignment{\ClassError{assignment}{No assignment number specified}{}}
% \newcommand*\students[2]{\gdef\@studenta{#1} \gdef\@studentb{#2}}
% \gdef\@studenta{\ClassError{studenta}{No student 1 specified}{}}
% \gdef\@studentb{\ClassError{studentb}{No student 2 specified}{}}

% \newcommand*\student2[1]{\gdef\@student2{#1}}
% \gdef\@student1{\ClassError{student1}{No student 1 specified}{}}

% \title{\textbf{LINGI2261 - Assignment 1}}
% \author{Group N°: .....\\ \\
% Name Student 1:  ..........\\ \\
% % Email Student 1: ..........\\
% Name Student 2:  ..........\\
% % Email Student 2: ..........\\
% }

% \makeatletter
% \def\@maketitle%
%   {
%   \begin{center}%
%   \let \footnote \thanks
%      {\Large\bf LINGI2261: Assignment 1 \\
%     %   \@docname\ \@assignment: \@title \par
%       }%
%      \def\and{\unskip, }
%      Student 1: \@student1\\
%      Student 2: \@student2\\
%      \@date
%   \end{center}%
%   \par
%   \vskip 1em
%   }
%   \makeatother

\makeatletter
\def\@maketitle{%
  \newpage
  \null
  \vskip 2em%
  \begin{center}%
  \let \footnote \thanks
    {\LARGE\bf \@docname \ LINFO1361: Assignment \@assignment  \par}%
    \vskip 1.5em%
    {\large
      \lineskip .5em%
      {\bf Group N°\groupnumber \\[.4cm]}
      \begin{tabular}[t]{ll}%
        Student1:& \studenta \\[.2cm]
        Student2:& \studentb
      \end{tabular}\par}%
    \vskip 1em%
    {\large \@date}%
  \end{center}%
  \par
  \vskip 1.5em}
\makeatother

\makeatletter
\let\@noitemerr\relax
\makeatother

% \newcommand\answer[1]{
% \begin{tikzpicture}
% \node (0,0) {
% \fbox{
% \parbox[c][3cm][c]{16.3cm}{%\centering%
%     % Your answer here
%     #1
% \hfill
% }}};
% \end{tikzpicture}}
 
\NewEnviron{answer}{%
\begin{tikzpicture}
\node (0,0) {
\fbox{
\parbox[c][3cm][c]{16.3cm}{%\centering%
    % Your answer here
    \BODY
\hfill
  }}};
\end{tikzpicture}}

\NewEnviron{answers}[1][]{%
\begin{tikzpicture}
\node (0,0) {
\fbox{
\parbox[c][#1][c]{16.3cm}{%\centering%
    % Your answer here
    \BODY
\hfill
  }}};
\end{tikzpicture}}

\NewEnviron{smallanswers}{%
\begin{tikzpicture}
\node (0,0) {
\fbox{
\parbox[c][.7cm][c]{3cm}{%\centering%
    % Your answer here
    \BODY
\hfill
  }}};
\end{tikzpicture}}