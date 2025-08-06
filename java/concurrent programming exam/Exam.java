import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.text.BreakIterator;
import java.util.*;
import java.util.concurrent.*;
import java.util.stream.Collector;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/*
This is the exam for DM563 - Concurrent Programming, Spring 2021.
Your task is to implement the following methods of class Exam:
- findUniqueWords;
- findCommonWords;
- wordLongerThan.
These methods search text files for particular words.
You must use a BreakIterator to identify words in a text file,
which you can obtain by calling BreakIterator.getWordInstance().
For more details on the usage of BreakIterator, please see the corresponding video lecture in the course.
The implementations of these methods must exploit concurrency to achieve improved performance.
The only code that you can change is the implementation of these methods.
In particular, you cannot change the signatures (return type, name, parameters) of any method, and you cannot edit method main.
The current code of these methods throws an UnsupportedOperationException: remove that line before proceeding on to the implementation.
You can find a complete explanation of the exam rules at the following webpage.
https://github.com/fmontesi/cp2021/tree/master/exam
@author Andreas Twisttmann Askholm <aaskh20>
*/
public class Exam {
    // Do not change this method
    public static void main(String[] args) {
        checkArguments(args.length > 0,
                "You must choose a command: help, findUniqueWords, findCommonWords, or wordLongerThan.");
        switch (args[0]) {
            case "help":
                System.out.println(
                        "Available commands: help, findUniqueWords, findCommonWords, or wordLongerThan.\nFor example, try:\n\tjava Exam findUniqueWords data");
                break;
            case "findUniqueWords":
                checkArguments(args.length == 2, "Usage: java Exam.java findUniqueWords <directory>");
                List<LocatedWord> uniqueWords = findUniqueWords(Paths.get(args[1]));
                System.out.println("Found " + uniqueWords.size() + " unique words");
                uniqueWords.forEach(locatedWord -> System.out.println(locatedWord.word + ":" + locatedWord.filepath));
                break;
            case "findCommonWords":
                checkArguments(args.length == 2, "Usage: java Exam.java findCommonWords <directory>");
                List<String> commonWords = findCommonWords(Paths.get(args[1]));
                System.out.println("Found " + commonWords.size() + " words in common");
                commonWords.forEach(System.out::println);
                break;
            case "wordLongerThan":
                checkArguments(args.length == 3, "Usage: java Exam.java wordLongerThan <directory> <length>");
                int length = Integer.parseInt(args[2]);
                Optional<LocatedWord> longerWordOptional = wordLongerThan(Paths.get(args[1]), length);
                longerWordOptional.ifPresentOrElse(
                        locatedWord -> System.out.println("Found " + locatedWord.word + " in " + locatedWord.filepath),
                        () -> System.out.println("No word found longer than " + args[2]));
                break;
            default:
                System.out.println("Unrecognised command: " + args[0] + ". Try java Exam.java help.");
                break;
        }
    }

    // Do not change this method
    private static void checkArguments(Boolean check, String message) {
        if (!check) {
            throw new IllegalArgumentException(message);
        }
    }

    /**
     * this method will be used to extract the unique words of a given file,
     * like the extractWords method from the Words class used in lectures and excercises
     * since the methods of the class only care about wether the word is there or not,
     * a hashset is used for the stream.
     * I decided to make this as a seperate method, since it will be used in all the other methods
     */
    private static Stream<String> getWords(String s) {

        HashSet<String> uwords = new HashSet<>();

        BreakIterator it = BreakIterator.getWordInstance();
        it.setText(s);

        int start = it.first();
        int end = it.next();
        while (end != BreakIterator.DONE) {
            String word = s.substring(start, end);

            if (Character.isLetterOrDigit(word.charAt(0))) {
                uwords.add(word.toLowerCase());
            }
            start = end;
            end = it.next();
        }

        return uwords.stream();

    }

    /**
     * Just like getWords above these lines of code will be used in more than one method,
     * so i decided to make it a seperate method.
     * this method uses the getWords method, to map each unique word to one or more files.
     *
     * @param Path    to a file
     * @param wordMap to store the word and its path(s)
     */
    private static void listWords(Path textFile, Map<String, Set<Path>> wordMap) {

        try {
            Files.lines(textFile)
                    .flatMap(Exam::getWords)
                    .forEach(s -> {
                        wordMap.merge(s, Set.of(textFile), (setAlreadyinMap, setOfPath) -> {
                            Set<Path> newSet = new HashSet<>(setAlreadyinMap);
                            newSet.add(textFile);
                            return newSet;
                        });
                    });
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Returns all the words that appear in at most one file among all the text
     * files contained in the given directory.
     * <p>
     * This method recursively visits a directory to find text files contained in it
     * and its subdirectories (and the subdirectories of these subdirectories,
     * etc.).
     * <p>
     * You must consider only files ending with a ".txt" suffix. You are guaranteed
     * that they will be text files.
     * <p>
     * The method should return a list of LocatedWord objects (defined by the class
     * at the end of this file). Each LocatedWord object should consist of: - a word
     * that appears in exactly one file (that is, the word must appear in at least
     * one file, but not more than one); - the path to the file containing the word.
     * <p>
     * All unique words must appear in the list: words that can be in the list must
     * be in the list.
     * <p>
     * Words must be considered equal without considering differences between
     * uppercase and lowercase letters. For example, the words "Hello", "hEllo" and
     * "HELLo" must be considered equal to the word "hello".
     *
     * @param dir the directory to search
     * @return a list of words unique to a single file
     */
    private static List<LocatedWord> findUniqueWords(Path dir) {

        Map<String, Set<Path>> wordMap = new ConcurrentHashMap<>();
        List<LocatedWord> unique = new ArrayList<>();

        ExecutorService exec = Executors.newWorkStealingPool();

        try {
            Files.walk(dir)
                    .filter(Files::isRegularFile)
                    .filter(path -> path.toString().endsWith(".txt"))
                    .forEach(filename -> exec.submit(() -> {
                        listWords(filename, wordMap);
                    }));


            exec.shutdown();
            exec.awaitTermination(1, TimeUnit.DAYS);


        } catch (InterruptedException | IOException e) {
            e.printStackTrace();
        }

        //for each word check the number of files it occurs in, if if occurs in only one file add a new located word to the list of located words
        wordMap.forEach((word, listOfPath) -> {
            if (listOfPath.size() == 1) {
                unique.add(new LocatedWord(word, Paths.get(listOfPath.toString())));
            }
        });

        return unique;
    }

    /**
     * Returns the words that appear at least once in every text file contained in
     * the given directory.
     * <p>
     * This method recursively visits a directory to find text files contained in it
     * and its subdirectories (and the subdirectories of these subdirectories,
     * etc.).
     * <p>
     * You must consider only files ending with a ".txt" suffix. You are guaranteed
     * that they will be text files.
     * <p>
     * The method should return a list of words, where each word appears at least once in
     * every file contained in the given directory.
     * <p>
     * Words must be considered equal without considering differences between
     * uppercase and lowercase letters. For example, the words "Hello", "hEllo" and
     * "HELLo" must be considered equal to the word "hello".
     *
     * @param dir the directory to search
     * @return a list of words common to all the files
     */
    private static List<String> findCommonWords(Path dir) {


        Map<String, Set<Path>> wordMap = new ConcurrentHashMap<>();
        List<String> commonWords = new ArrayList<>();

        ExecutorService exec = Executors.newWorkStealingPool();

        try {
            List<Path> pathList = Files.walk(dir)
                    .filter(Files::isRegularFile)
                    .filter(path -> path.toString().endsWith(".txt"))
                    .collect(Collectors.toList());


            pathList.stream()
                    .forEach(filename -> exec.submit(() -> {
                                listWords(filename, wordMap);
                            })
                    );


            exec.shutdown();
            exec.awaitTermination(1, TimeUnit.DAYS);

            //for each word check the number of files it occurs in, if if occurs in all files add it to the list of commonwords
            wordMap.forEach((word, listOfPath) -> {
                if (listOfPath.size() == pathList.size()) {
                    commonWords.add(word);

                }

            });

        } catch (InterruptedException | IOException e) {
            e.printStackTrace();
        }

        return commonWords;
    }

    /**
     * Returns an Optional<LocatedWord> (see below) about a word found in the files
     * of the given directory whose length is greater than the given length.
     * <p>
     * This method recursively visits a directory to find text files contained in it
     * and its subdirectories (and the subdirectories of these subdirectories,
     * etc.).
     * <p>
     * You must consider only files ending with a ".txt" suffix. You are guaranteed
     * that they will be text files.
     * <p>
     * The method should return an (optional) LocatedWord object (defined by the
     * class at the end of this file), consisting of:
     * - the word found that is longer than the given length;
     * - the path to the file containing the word.
     * <p>
     * If a word satisfying the description above can be found, then the method
     * should return an Optional containing the desired LocatedWord. Otherwise, if
     * such a word cannot be found, the method should return Optional.empty().
     * <p>
     * This method should return *as soon as possible*: as soon as a satisfactory
     * word is found, the method should return a result without waiting for the
     * processing of remaining files and/or other data.
     *
     * @param dir    the directory to search
     * @param length the length the word searched for must exceed
     * @return an optional LocatedWord about a word longer than the given length
     */
    private static Optional<LocatedWord> wordLongerThan(Path dir, int length) {

        Optional<LocatedWord> foundword = Optional.empty();
        ExecutorService exec = Executors.newWorkStealingPool();

        ExecutorCompletionService<LocatedWord> completionService =
                new ExecutorCompletionService<>(exec);

        try {

            List<Future<LocatedWord>> futures =
                    Files.walk(dir)
                            .filter(Files::isRegularFile)
                            .filter(path -> path.toString().endsWith(".txt"))
                            .map(filepath -> completionService
                                    .submit(() ->
                                            discoverWords(filepath, length)
                                    )
                            )
                            .collect(Collectors.toList());

            exec.shutdown();

            //go through the list of located words, if a word is found stop all other threads and return that word
            for (int i = futures.size(); i > 0; i--) {
                try {
                    LocatedWord result = completionService.take().get();

                    if (result.word.length() > length) {
                        futures.forEach(f -> f.cancel(true));
                        foundword = Optional.of(result);
                    }
                } catch (ExecutionException | InterruptedException | CancellationException e) {
                    //System.err.println(e);
                    //prints a message when a future is cancelled or any other exception is caught
                    //commented out since we do not need or want to know when a thread is stopped
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }


        return foundword;

    }

    /**
     * This method is used to find the first word in a stream, that exceeds the given length
     * it returns a located word for each file, some might have an empty string which is handled in wordLongerThan
     */
    private static LocatedWord discoverWords(Path filepath, int length) {

        String word = "";

        try {
            word = Files.lines(filepath)
                    .flatMap(Exam::getWords)
                    .filter(s -> s.length() > length)
                    .findFirst().orElse("");

        } catch (IOException e) {
            e.printStackTrace();
        }
        return new LocatedWord(word, filepath);
    }

    // Do not change this class
    private static class LocatedWord {
        private final String word; // the word
        private final Path filepath; // the file where the word has been found

        private LocatedWord(String word, Path filepath) {
            this.word = word;
            this.filepath = filepath;
        }
    }

    // Do not change this class
    private static class WordLocation {
        private final Path filepath; // the file where the word has been found
        private final int line; // the line number at which the word has been found

        private WordLocation(Path filepath, int line) {
            this.filepath = filepath;
            this.line = line;
        }
    }

    // Do not change this class
    private static class InternalException extends RuntimeException {
        private InternalException(String message) {
            super(message);
        }
    }
}