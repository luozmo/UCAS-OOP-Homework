package libraryexample;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Student {
    private final String studentId;
    private final String name;
    private final String college;
    private final int maxBorrowCount;
    private final List<Book> borrowedBooks;

    public Student(String studentId, String name, String college, int maxBorrowCount) {
        if (maxBorrowCount < 0) {
            throw new IllegalArgumentException("maxBorrowCount cannot be negative");
        }
        this.studentId = studentId;
        this.name = name;
        this.college = college;
        this.maxBorrowCount = maxBorrowCount;
        this.borrowedBooks = new ArrayList<Book>();
    }

    public String getStudentId() {
        return studentId;
    }

    public String getName() {
        return name;
    }

    public String getCollege() {
        return college;
    }

    public int getMaxBorrowCount() {
        return maxBorrowCount;
    }

    public int getBorrowedCount() {
        return borrowedBooks.size();
    }

    public List<Book> getBorrowedBooks() {
        return Collections.unmodifiableList(new ArrayList<Book>(borrowedBooks));
    }

    public boolean borrowBook(Book book) {
        if (book == null || !book.isAvailable()) {
            return false;
        }
        if (borrowedBooks.size() >= maxBorrowCount) {
            return false;
        }
        if (!book.decreaseAvailableCopy()) {
            return false;
        }
        borrowedBooks.add(book);
        return true;
    }

    public boolean returnBook(Book book) {
        if (book == null || !borrowedBooks.remove(book)) {
            return false;
        }
        book.increaseAvailableCopy();
        return true;
    }

    @Override
    public String toString() {
        return name + " (" + studentId + "), borrowed books: " + borrowedBooks.size();
    }
}
